/* bdf2pxl.c - Convert BDF font to pxl_font_t
 *
 * Usage:
 * bdf2pxl [-n name] [-r U+START-U+END] [-s] font.bdf
 *
 * Output: pxl_font_t C header to stdout
 *
 * Options:
 * -n name    Font name for header guard (default: basename of font file)
 * -r range   Unicode range (e.g. U+0020-U+007E). Auto-detected if omitted.
 * -s         Subfont mode: output only the font data (no header guards)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_LINE_LEN 1024
#define PXL_CALC_STRIDE(w) (((w) + 7) / 8)

typedef struct {
	const char *name;
	const char *source;
	int font_size;

	uint8_t *bitmask;
	int num_chars;
	int glyph_w;
	int glyph_h;
	size_t stride;

	uint32_t rune_start;
	uint32_t rune_end;
	uint32_t fallback_rune;

	uint8_t *glyph_widths;
	uint8_t *glyph_advances;
	int8_t *glyph_offsets_x;
	int8_t *glyph_offsets_y;

	int tracking;
	int leading;
	int glyph_height;
} pxl_font_header_data_t;

static inline char *
get_basename_noext(const char *path)
{
	if (!path || !*path) return NULL;
	
	const char *base = strrchr(path, '/');
	base = base ? base + 1 : path;
	
	const char *dot = strrchr(base, '.');
	size_t len = dot ? (size_t)(dot - base) : strlen(base);
	
	char *result = malloc(len + 1);
	if (result) {
		memcpy(result, base, len);
		result[len] = '\0';
	}
	return result;
}

static inline uint32_t
parse_codepoint(const char *s) {
	int base = 16;
	if (s[0] == 'U' && s[1] == '+') s += 2;
	else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
	else base = 0;
	return (uint32_t)strtoul(s, NULL, base);
}

static inline int
parse_range(const char *str, uint32_t *start, uint32_t *end) {
	char *dash = strchr(str, '-');
	if (!dash) {
		*start = *end = parse_codepoint(str);
	} else {
		*start = parse_codepoint(str);
		*end = parse_codepoint(dash + 1);
	}
	return (*end >= *start);
}

static inline void
generate_header(FILE *out, const pxl_font_header_data_t *d, bool subfont_only) {
	int total_h = d->num_chars * d->glyph_h;

	char upper_name[256];
	if (!subfont_only) {
		const char *base = d->source;
		const char *slash = strrchr(d->source, '/');
		if (slash) base = slash + 1;

		size_t i;
		for (i = 0; i < sizeof(upper_name) - 1 && d->name[i]; i++) {
			upper_name[i] = (char)toupper((unsigned char)d->name[i]);
		}
		upper_name[i] = '\0';

		fprintf(out, "/* Auto-generated from %s */\n\n", base);
		fprintf(out, "#ifndef PXL_FONT_%s_H\n", upper_name);
		fprintf(out, "#define PXL_FONT_%s_H\n\n", upper_name);
		fprintf(out, "#include <stdint.h>\n");
		fprintf(out, "#include \"text.h\"\n\n");
	}

	if (d->glyph_widths) {
		fprintf(out, "static const uint8_t %s_widths[%d] = {", d->name, d->num_chars);
		for (int i = 0; i < d->num_chars; i++) {
			fprintf(out, "%s%d", (i > 0) ? ", " : "\n    ", d->glyph_widths[i]);
		}
		fprintf(out, "\n};\n\n");
	}

	if (d->glyph_advances) {
		fprintf(out, "static const uint8_t %s_advances[%d] = {", d->name, d->num_chars);
		for (int i = 0; i < d->num_chars; i++) {
			fprintf(out, "%s%d", (i > 0) ? ", " : "\n    ", d->glyph_advances[i]);
		}
		fprintf(out, "\n};\n\n");
	}

	if (d->glyph_offsets_x) {
		fprintf(out, "static const int8_t %s_offsets_x[%d] = {", d->name, d->num_chars);
		for (int i = 0; i < d->num_chars; i++) {
			fprintf(out, "%s%d", (i > 0) ? ", " : "\n    ", d->glyph_offsets_x[i]);
		}
		fprintf(out, "\n};\n\n");
	}

	if (d->glyph_offsets_y) {
		fprintf(out, "static const int8_t %s_offsets_y[%d] = {", d->name, d->num_chars);
		for (int i = 0; i < d->num_chars; i++) {
			fprintf(out, "%s%d", (i > 0) ? ", " : "\n    ", d->glyph_offsets_y[i]);
		}
		fprintf(out, "\n};\n\n");
	}

	fprintf(out, "static const uint8_t %s_bitmask[%d][%d][%zu] = {\n",
		d->name, d->num_chars, d->glyph_h, d->stride);

	for (int c = 0; c < d->num_chars; c++) {
		uint32_t rune = d->rune_start + (uint32_t)c;
		fprintf(out, "    /* U+%04X */\n    {\n", rune);
		for (int y = 0; y < d->glyph_h; y++) {
			fprintf(out, "        {");
			for (size_t b = 0; b < d->stride; b++) {
				size_t offset = (size_t)c * (size_t)d->glyph_h * (size_t)d->stride + (size_t)y * (size_t)d->stride + b;
				fprintf(out, "0x%02X%s", d->bitmask[offset],
					(b < d->stride - 1) ? ", " : "");
			}
			fprintf(out, "},\n");
		}
		fprintf(out, "    },\n");
	}

	fprintf(out, "};\n\n");
	fprintf(out, "const pxl_font_t %s = {\n", d->name);
	fprintf(out, "    .bitmask = {.data = (const uint8_t *)%s_bitmask,\n", d->name);
	fprintf(out, "                .width = %d, .height = %d, .stride = %zu},\n",
		d->glyph_w, total_h, d->stride);
	fprintf(out, "    .rune_start = %u, .rune_end = %u,\n", d->rune_start, d->rune_end);
	fprintf(out, "    .fallback_rune = %u,\n", d->fallback_rune);
	fprintf(out, "    .tracking = %d, .leading = %d, .glyph_height = %d,\n",
		d->tracking, d->leading, d->glyph_height);
	/* Build array names or use NULL */
	char widths_buf[256] = {0};
	char advances_buf[256] = {0};
	char offsets_x_buf[256] = {0};
	char offsets_y_buf[256] = {0};
	
	const char *widths_ref = "NULL";
	const char *advances_ref = "NULL";
	const char *offsets_x_ref = "NULL";
	const char *offsets_y_ref = "NULL";
	
	if (d->glyph_widths) {
		snprintf(widths_buf, sizeof(widths_buf), "%s_widths", d->name);
		widths_ref = widths_buf;
	}
	if (d->glyph_advances) {
		snprintf(advances_buf, sizeof(advances_buf), "%s_advances", d->name);
		advances_ref = advances_buf;
	}
	if (d->glyph_offsets_x) {
		snprintf(offsets_x_buf, sizeof(offsets_x_buf), "%s_offsets_x", d->name);
		offsets_x_ref = offsets_x_buf;
	}
	if (d->glyph_offsets_y) {
		snprintf(offsets_y_buf, sizeof(offsets_y_buf), "%s_offsets_y", d->name);
		offsets_y_ref = offsets_y_buf;
	}
	
	fprintf(out, "    .glyph_widths = %s,\n", widths_ref);
	fprintf(out, "    .glyph_advances = %s,\n", advances_ref);
	fprintf(out, "    .glyph_offsets_x = %s,\n", offsets_x_ref);
	fprintf(out, "    .glyph_offsets_y = %s\n", offsets_y_ref);
	fprintf(out, "};\n");
	
	if (!subfont_only) {
		fprintf(out, "\n#endif /* PXL_FONT_%s_H */\n", upper_name);
	}
}

typedef struct {
	uint32_t codepoint;
	int w, h, xoff, yoff, adv;
	char **bitmap_lines;
	int num_bitmap_lines;
} bdf_glyph_t;

static void
free_glyph(bdf_glyph_t *g) {
	if (!g || !g->bitmap_lines) return;
	for (int i = 0; i < g->num_bitmap_lines; i++) {
		free(g->bitmap_lines[i]);
	}
	free(g->bitmap_lines);
	g->bitmap_lines = NULL;
	g->num_bitmap_lines = 0;
}

static int
parse_bdf(const char *path, bdf_glyph_t **glyphs_out, int *num_glyphs_out,
          int *ascent_out, int *font_bbox_h_out) {
	FILE *f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Error: Cannot open file '%s'\n", path);
		return 0;
	}

	bdf_glyph_t *glyphs = calloc(32, sizeof(bdf_glyph_t));
	if (!glyphs) { fclose(f); return 0; }

	int num = 0, cap = 32, asc = 0, bbox_h = 16, in_char = 0;
	char line[MAX_LINE_LEN];
	bdf_glyph_t *cur = NULL;

	while (fgets(line, sizeof(line), f)) {
		line[strcspn(line, "\r\n")] = 0;

		if (strncmp(line, "FONT_ASCENT", 11) == 0)
		   	asc = atoi(line + 11);
		else if (strncmp(line, "FONTBOUNDINGBOX", 15) == 0) {
			char *t = strtok(line + 15, " ");
			if (t) t = strtok(NULL, " ");
		   	if (t) bbox_h = atoi(t);
		} else if (strncmp(line, "FONTBOUNDS", 10) == 0) {
			char *t = strtok(line + 10, " ");
			if (t) t = strtok(NULL, " "); if (t) t = strtok(NULL, " "); if (t) bbox_h = atoi(t);
		} else if (strncmp(line, "ENCODING", 8) == 0) {
			if (num >= cap) {
				cap *= 2;
				bdf_glyph_t *ng = realloc(glyphs, (size_t)cap * sizeof(bdf_glyph_t));
				if (!ng) {
				   	for (int i = 0; i < num; i++) {
					   	free_glyph(&glyphs[i]);
					}
				   	free(glyphs);
				   	fclose(f);
				   	return 0;
			   	}
				glyphs = ng;
			}
			cur = &glyphs[num++];
			memset(cur, 0, sizeof(bdf_glyph_t)); /* Crucial for realloc safety */
			cur->codepoint = (uint32_t)atoi(line + 8);
			in_char = 1;
		} else if (in_char && strncmp(line, "BBX", 3) == 0) {
			char *t = strtok(line + 3, " "); if (t) cur->w = atoi(t);
			if ((t = strtok(NULL, " "))) cur->h = atoi(t);
			if ((t = strtok(NULL, " "))) cur->xoff = atoi(t);
			if ((t = strtok(NULL, " "))) cur->yoff = atoi(t);
		} else if (in_char && strncmp(line, "DWIDTH", 6) == 0) {
			char *t = strtok(line + 6, " ");
		   	if (t) cur->adv = atoi(t);
		} else if (in_char && strncmp(line, "BITMAP", 6) == 0) {
			cur->bitmap_lines = NULL;
		   	cur->num_bitmap_lines = 0;
		} else if (in_char && strncmp(line, "ENDCHAR", 7) == 0) {
			in_char = 0;
		} else if (in_char && cur && cur->w > 0) {
			if (cur->num_bitmap_lines >= cur->h) continue;
			char **nl = realloc(cur->bitmap_lines, (size_t)(cur->num_bitmap_lines + 1) * sizeof(char *));
			if (!nl) {
			   	for (int i = 0; i < num; i++) {
				   	free_glyph(&glyphs[i]);
				}
				free(glyphs);
			   	fclose(f);
			   	return 0;
		   	}
			cur->bitmap_lines = nl;
			cur->bitmap_lines[cur->num_bitmap_lines++] = strdup(line);
		}
	}
	fclose(f);
	*ascent_out = asc; *font_bbox_h_out = bbox_h;
	*glyphs_out = glyphs; *num_glyphs_out = num;
	return 1;
}

static void
hex_line_to_lsb(const char *hex, int w, uint8_t *out, size_t stride) {
	memset(out, 0, stride);
	uint32_t val = (uint32_t)strtoul(hex, NULL, 16);
	int bits = (int)strlen(hex) * 4;
	for (int x = 0; x < w; x++) {
		int shift = bits - 1 - x;
		if (shift >= 0 && shift < 32 && ((val >> shift) & 1))
			out[x / 8] |= (1 << (x % 8));
	}
}

int
main(int argc, char **argv) {
	int opt;
	char *name = NULL, *range = NULL;
	bool subfont_only = false;

	while ((opt = getopt(argc, argv, "n:r:s")) != -1) {
		switch (opt) {
			case 'n': 
				name = strdup(optarg);
				if (!name) {
				   	fprintf(stderr, "Error: Out of memory\n");
				   	return 1;
			   	}
				break;
			case 'r': range = optarg; break;
			case 's': subfont_only = true; break;
			default:
				fprintf(stderr, "Usage: %s [-n name] [-r range] [-s] font.bdf\n", argv[0]);
				return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Error: No font file specified\n");
		fprintf(stderr, "Usage: %s [-n name] [-r range] font.bdf\n", argv[0]);
		free(name);
		return 1;
	}

	const char *font_path = argv[optind];

	if (!name) {
		name = get_basename_noext(font_path);
		if (!name) {
			fprintf(stderr, "Error: Could not determine font name from '%s'\n", font_path);
			return 1;
		}
	}

	bdf_glyph_t *glyphs = NULL;
	int num_glyphs = 0;
	int ascent = 0;
	int bbox_h = 16;

	if (!parse_bdf(font_path, &glyphs, &num_glyphs, &ascent, &bbox_h)) {
		free(name);
		return 1;
	}

	/* Warn if font file contains no glyphs */
	if (num_glyphs == 0) {
		fprintf(stderr, "Warning: Font file contains no valid glyphs\n");
	}

	/* 1. Establish strict continuous bounds */
	uint32_t r_start = 0xFFFFFFFF, r_end = 0;
	if (range && *range) {
		if (!parse_range(range, &r_start, &r_end)) {
			fprintf(stderr, "Error: Invalid range list '%s'\n", range);
			for (int i = 0; i < num_glyphs; i++) free_glyph(&glyphs[i]);
			free(glyphs); free(name); return 1;
		}
	} else {
		for (int i = 0; i < num_glyphs; i++) {
			if (glyphs[i].codepoint < r_start) r_start = glyphs[i].codepoint;
			if (glyphs[i].codepoint > r_end) r_end = glyphs[i].codepoint;
		}
		/* Handle empty font (no glyphs parsed) */
		if (num_glyphs == 0) {
			r_start = 0;
			r_end = 0;
		}
	}
	
	/* 2. Setup Exact Allocations for continuous lookup (O(1)) */
	pxl_font_header_data_t d = {0};
	d.source = font_path; d.name = name; d.font_size = bbox_h;
	d.rune_start = r_start; d.rune_end = r_end;

	/* Warn if font range is invalid (would cause underflow) */
	if (r_end < r_start) {
		fprintf(stderr, "Warning: Invalid font range U+%04X-U+%04X, adjusting to empty range\n",
			r_start, r_end);
		r_start = 0;
		r_end = 0;
	}

	d.num_chars = (int)(r_end - r_start + 1);
	d.fallback_rune = r_start;

	/* Warn if font range is empty */
	if (d.num_chars <= 0) {
		fprintf(stderr, "Warning: Empty font range U+%04X-U+%04X (%d characters)\n",
			r_start, r_end, d.num_chars);
	}

	int max_w = 0;
	for (int i = 0; i < num_glyphs; i++) if (glyphs[i].w > max_w) max_w = glyphs[i].w;
	d.glyph_w = (max_w == 0) ? 8 : max_w;
	d.glyph_h = bbox_h;
	d.stride = (size_t)PXL_CALC_STRIDE(d.glyph_w);

	d.glyph_widths = calloc((size_t)d.num_chars, sizeof(uint8_t));
	d.glyph_advances = calloc((size_t)d.num_chars, sizeof(uint8_t));
	d.glyph_offsets_x = calloc((size_t)d.num_chars, sizeof(int8_t));
	d.glyph_offsets_y = calloc((size_t)d.num_chars, sizeof(int8_t));
	d.bitmask = calloc((size_t)d.num_chars * (size_t)d.glyph_h * d.stride, 1);

	/* 3. Fill data. Missing glyphs stay perfectly 0-filled */
	for (int c = 0; c < d.num_chars; c++) {
		uint32_t rune = d.rune_start + (uint32_t)c;
		bdf_glyph_t *g = NULL;
		
		for (int i = 0; i < num_glyphs; i++) {
			if (glyphs[i].codepoint == rune) { g = &glyphs[i]; break; }
		}

		if (g) {
			d.glyph_widths[c] = (uint8_t)g->w;
			d.glyph_advances[c] = (uint8_t)g->adv;
			d.glyph_offsets_x[c] = (int8_t)g->xoff;
			d.glyph_offsets_y[c] = (int8_t)(ascent - g->h - g->yoff);

			if (g->w > 0 && g->num_bitmap_lines > 0) {
				int y_start = ascent - g->h - g->yoff;
				for (int y = 0; y < d.glyph_h; y++) {
					uint8_t *row = d.bitmask + (size_t)c * (size_t)d.glyph_h * d.stride + (size_t)y * d.stride;
					if (y >= y_start && y < y_start + g->h) {
						int by = y - y_start;
						if (by < g->num_bitmap_lines) hex_line_to_lsb(g->bitmap_lines[by], g->w, row, d.stride);
					}
				}
			}
		} else {
			fprintf(stderr, "Warning: Codepoint U+%04X missing (padded with 0)\n", rune);
		}
	}

	/* 4. Optimize: replace arrays with NULL if all values match defaults */
	
	/* Offsets X: all zero? */
	bool offsets_x_all_zero = true;
	for (int i = 0; i < d.num_chars; i++) {
		if (d.glyph_offsets_x[i] != 0) {
			offsets_x_all_zero = false;
			break;
		}
	}
	if (offsets_x_all_zero) {
		free(d.glyph_offsets_x);
		d.glyph_offsets_x = NULL;
	}

	/* Offsets Y: all zero? */
	bool offsets_y_all_zero = true;
	for (int i = 0; i < d.num_chars; i++) {
		if (d.glyph_offsets_y[i] != 0) {
			offsets_y_all_zero = false;
			break;
		}
	}
	if (offsets_y_all_zero) {
		free(d.glyph_offsets_y);
		d.glyph_offsets_y = NULL;
	}

	/* Widths: all equal to glyph_w? */
	bool widths_all_same = true;
	for (int i = 0; i < d.num_chars; i++) {
		if (d.glyph_widths[i] != (uint8_t)d.glyph_w) {
			widths_all_same = false;
			break;
		}
	}
	if (widths_all_same) {
		free(d.glyph_widths);
		d.glyph_widths = NULL;
	}

	/* Advances: all equal to default? */
	bool advances_all_default = true;
	if (d.glyph_widths == NULL) {
		/* If widths is NULL, default advance is glyph_w */
		for (int i = 0; i < d.num_chars; i++) {
			if (d.glyph_advances[i] != (uint8_t)d.glyph_w) {
				advances_all_default = false;
				break;
			}
		}
	} else {
		/* If widths exists, default advance is widths[i] */
		for (int i = 0; i < d.num_chars; i++) {
			if (d.glyph_advances[i] != d.glyph_widths[i]) {
				advances_all_default = false;
				break;
			}
		}
	}
	if (advances_all_default) {
		free(d.glyph_advances);
		d.glyph_advances = NULL;
	}

	for (int i = 0; i < num_glyphs; i++) free_glyph(&glyphs[i]);
	free(glyphs);

	d.tracking = 1; d.leading = d.glyph_h; d.glyph_height = d.glyph_h;
	generate_header(stdout, &d, subfont_only);

	free(d.bitmask); free(d.glyph_widths); free(d.glyph_advances);
	free(d.glyph_offsets_x); free(d.glyph_offsets_y); free(name);
	return 0;
}
