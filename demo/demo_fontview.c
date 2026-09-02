/*
 * PXL Demo: Font Viewer
 *
 * Static font visualization demo showcasing PXL's font system:
 *   - Font loading and rendering (pxl_font_t, pxl_writer_t)
 *   - Bitmask font rendering
 *   - Canvas with scissor regions
 *   - UTF-8/rune support
 *
 * This is a STATIC demo (no physics loop) using pxl_app_advance_wait().
 * For interactive demos with movement/physics, see demo_pong.c.
 *
 * Note: font_9x15.h and font_wqy_13pts.h are generated font headers (see tool/bdf2pxl).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "pxl.h"
#include "demo_helpers.h"

#include "font_9x15.h"
#include "font_wqy_13pts.h"

/* Font families */
static const pxl_font_t *font_family_latin[] = {
	&font_9x15_latin
};

static const pxl_font_t *font_family_wqy_13pts[] = {
	&wqy_13pts_cjk,
	&wqy_13pts_punct,
	&wqy_13pts_punct_fw
};

static const pxl_font_t **font_families[] = {
	font_family_latin,
	font_family_wqy_13pts
};

static const size_t font_family_sizes[] = {
	1,  /* latin */
	3   /* wqy 13pts */
};

static const char *font_family_names[] = {
	"Latin 9x15",
	"CJK WQY 13pts"
};

#define NUM_FONT_FAMILIES (sizeof(font_families) / sizeof(font_families[0]))

#define W 720
#define H 360

#define BG                   0xFFFDF6E3  /* Solarized Base3 */
#define GRID_VIEW_BG         0xFFEEE8D5  /* Solarized Base2 */
#define GRID_VIEW_FG         0xFF657B83  /* Solarized Base0 */
#define GRID_VIEW_SELECT_FG  0xFF268BD2  /* Solarized Blue */
#define SCROLLBAR_BG         GRID_VIEW_BG
#define SCROLLBAR_FG         GRID_VIEW_FG
#define TITLE_BG             GRID_VIEW_BG 
#define TITLE_FG             GRID_VIEW_FG
#define GLYPH_ZOOM_BG        GRID_VIEW_BG
#define GLYPH_ZOOM_FG        GRID_VIEW_SELECT_FG
#define GLYPH_BG             GRID_VIEW_BG
#define GLYPH_FG             GRID_VIEW_SELECT_FG
#define FOOTER_BG            GRID_VIEW_BG
#define FOOTER_FG            GRID_VIEW_FG

/* Lorem ipsum texts for each font family */
static const char *lorem_texts[] = {
    "Lorem ipsum dolor sit amet consectetur adipiscing elit sed do",
    "中文字体测试文本用于展示字体效果和排版"
};

#define W_PADDING 8
#define H_PADDING 16

#define TITLE_X GRID_VIEW_X
#define TITLE_Y ((H - TITLE_H - H_PADDING - GRID_H - H_PADDING - GLYPH_ZOOM_H - H_PADDING - TEXT_PREVIEW_H - H_PADDING - FOOTER_H) / 2)
#define TITLE_W GRID_W
#define TITLE_H 16

#define GRID_VIEW_CELL_W 16
#define GRID_VIEW_CELL_H 16
#define GRID_VIEW_COLS  35
#define GRID_VIEW_ROWS   7
#define GRID_VIEW_X  (W - GRID_W) / 2
#define GRID_VIEW_Y  (TITLE_Y + TITLE_H + H_PADDING)
#define GRID_VIEW_W  (GRID_VIEW_COLS * GRID_VIEW_CELL_W)
#define GRID_VIEW_H  (GRID_VIEW_ROWS * GRID_VIEW_CELL_H)

#define SCROLLBAR_X (GRID_VIEW_X + GRID_VIEW_W + W_PADDING)
#define SCROLLBAR_Y GRID_VIEW_Y
#define SCROLLBAR_W 8
#define SCROLLBAR_H GRID_VIEW_H

#define GRID_W (GRID_VIEW_W + W_PADDING + SCROLLBAR_W)
#define GRID_H GRID_VIEW_H

#define GLYPH_ZOOM_FACTOR 3
#define GLYPH_ZOOM_X GRID_VIEW_X
#define GLYPH_ZOOM_Y (GRID_VIEW_Y + GRID_H + H_PADDING)
#define GLYPH_ZOOM_W 60
#define GLYPH_ZOOM_H 60

#define GLYPH_X (GLYPH_ZOOM_X + GLYPH_ZOOM_W + W_PADDING)
#define GLYPH_Y GLYPH_ZOOM_Y
#define GLYPH_W (GRID_W - GLYPH_ZOOM_W - W_PADDING)
#define GLYPH_H GLYPH_ZOOM_H

#define TEXT_PREVIEW_X    GRID_VIEW_X
#define TEXT_PREVIEW_Y    (GLYPH_Y + GLYPH_H + H_PADDING)
#define TEXT_PREVIEW_W    GRID_W
#define TEXT_PREVIEW_H    40
#define TEXT_PREVIEW_BG   GRID_VIEW_BG
#define TEXT_PREVIEW_FG   GRID_VIEW_FG

#define FOOTER_X GRID_VIEW_X
#define FOOTER_Y (TEXT_PREVIEW_Y + TEXT_PREVIEW_H + H_PADDING)
#define FOOTER_W GRID_W
#define FOOTER_H TITLE_H


static void
fmt_mem_size(size_t bytes, char *buf, size_t buf_size) {
	if (bytes < 1024) {
		snprintf(buf, buf_size, "%zu B", bytes);
	} else if (bytes < 1024 * 1024) {
		snprintf(buf, buf_size, "%.1f KiB", (double)bytes / 1024.0);
	} else if (bytes < 1024 * 1024 * 1024) {
		snprintf(buf, buf_size, "%.1f MiB", (double)bytes / (1024.0 * 1024.0));
	} else {
		snprintf(buf, buf_size, "%.1f GiB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
	}
}

typedef struct {
	const pxl_bitmask_t *bitmask;
	pxl_rect_t bitmask_r;
	uint32_t codepoint;
	int      width;
	int      height;
	int      offset_x;
	int      offset_y;
	int      advance;
} glyph_t;

typedef struct {
	int cols;               /* Number of columns in grid */
	int rows;               /* Number of rows in grid */
	int family_idx;         /* Current font family index */
	int glyph_idx;          /* Index of selected glyph in current view */

	/* Internal */
	int   glyph_per_page;
	int   start_idx;
	int   end_idx;

	const pxl_font_t **font_family;
	int               font_family_size;
	const char       *font_family_name;
	int               font_family_glyph_count;
} font_view_t;

static int
font_view_glyph(const font_view_t *fv, int idx, glyph_t *out_glyph) {
	assert(out_glyph);
	assert(idx >= 0 && idx < fv->font_family_glyph_count);

	const pxl_font_t **family_fonts = font_families[fv->family_idx];
	int idx_in_font = idx;

	*out_glyph = (glyph_t){0};

	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = family_fonts[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);

		if (idx_in_font < font_count) {
			out_glyph->bitmask   = &font->bitmask;
			out_glyph->codepoint = (uint32_t)font->rune_start + (uint32_t)idx_in_font;
			out_glyph->width     = font->bitmask.width;
			out_glyph->height    = font->glyph_height;

			/* Per-glyph metrics if available */
			if (font->glyph_widths) {
				out_glyph->width = font->glyph_widths[idx_in_font];
			}
			if (font->glyph_offsets_x) {
				out_glyph->offset_x = font->glyph_offsets_x[idx_in_font];
			}
			if (font->glyph_offsets_y) {
				out_glyph->offset_y = font->glyph_offsets_y[idx_in_font];
			}
			if (font->glyph_advances) {
				out_glyph->advance = font->glyph_advances[idx_in_font];
			}

			out_glyph->bitmask_r = (pxl_rect_t){
				.y = idx_in_font * out_glyph->height,
			   	.w = out_glyph->width,
			   	.h = out_glyph->height
			};

			return 0;
		}
		idx_in_font -= font_count;
		assert(idx_in_font >= 0);
	}

	/* Fallback (should never happen due to assert) */
	return 1;
}

static void
font_view_next_glyph(font_view_t *fv, int idx_inc) {
	int new_idx = fv->glyph_idx + idx_inc;
	if (new_idx >= 0 && new_idx < fv->font_family_glyph_count) {
		fv->glyph_idx = new_idx;
	}

	fv->start_idx = (fv->glyph_idx / fv->glyph_per_page) * fv->glyph_per_page;
	fv->end_idx = fv->start_idx + fv->glyph_per_page;
	if (fv->end_idx > fv->font_family_glyph_count) fv->end_idx = fv->font_family_glyph_count;
}

static void
font_view_next_font(font_view_t *fv) {
	fv->family_idx = (int)((((size_t)fv->family_idx + 1u) % NUM_FONT_FAMILIES));

	fv->font_family      = font_families[fv->family_idx];
	fv->font_family_size = (int)font_family_sizes[fv->family_idx];
	fv->font_family_name = font_family_names[fv->family_idx];

	fv->font_family_glyph_count = 0;
	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = fv->font_family[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);
		fv->font_family_glyph_count += font_count;
	}

	fv->glyph_idx  = -1;
	font_view_next_glyph(fv, 1);
}

static void
font_view_init(font_view_t *fv) {
	fv->cols = GRID_VIEW_COLS;
	fv->rows = GRID_VIEW_ROWS;

	fv->glyph_per_page = fv->cols * fv->rows;

	fv->family_idx = -1;
	font_view_next_font(fv);
}

static size_t
font_view_mem_size(const font_view_t *fv) {
	size_t total_size = 0;
	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = fv->font_family[i];
		uint32_t glyph_count = font->rune_end - font->rune_start + 1;
		
		/* Bitmask data */
		total_size += (size_t)glyph_count * (size_t)font->glyph_height * (size_t)font->bitmask.stride;
		
		/* Metadata arrays (each is glyph_count elements) */
		if (font->glyph_widths) total_size += glyph_count * sizeof(uint8_t);
		if (font->glyph_advances) total_size += glyph_count * sizeof(uint8_t);
		if (font->glyph_offsets_x) total_size += glyph_count * sizeof(int8_t);
		if (font->glyph_offsets_y) total_size += glyph_count * sizeof(int8_t);
	}
	
	return total_size;
}

static void
handle_input(font_view_t *fv, pxl_app_t *app) {
	if (pxl_app_was_pressed(app, PXL_KEYB_F)) {
		font_view_next_font(fv);
	}

	if (pxl_app_was_pressed(app, PXL_KEYB_H) || pxl_app_is_pressed(app, PXL_KEYB_LEFT)) {
		font_view_next_glyph(fv, -1);
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_J) || pxl_app_is_pressed(app, PXL_KEYB_DOWN)) {
		font_view_next_glyph(fv, fv->cols);
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_K) || pxl_app_is_pressed(app, PXL_KEYB_UP)) {
		font_view_next_glyph(fv, -fv->cols);
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_L) || pxl_app_is_pressed(app, PXL_KEYB_RIGHT)) {
		font_view_next_glyph(fv, 1);
	}
}

static void
render_title(pxl_canvas_t *cnv, const font_view_t *fv) {
	char title_text[128];
	snprintf(title_text, sizeof(title_text), "PXL Font Viewer - %s", fv->font_family_name);
	pxl_rect_t title_bounds = pxl_str_bounds(title_text);

	pxl_canvas_set_color(cnv, TITLE_FG);
	/* Position relative to the title canvas (0,0 is top-left of subview) */
	int title_x = (TITLE_W - title_bounds.w) / 2;
	int title_y = (TITLE_H - title_bounds.h) / 2;
	pxl_draw_str(cnv, title_x, title_y, title_text);
}

static void
render_font_view(pxl_canvas_t *cnv, const font_view_t *fv) {
	pxl_canvas_set_color(cnv, GRID_VIEW_SELECT_FG);
	/* Draw selection rectangle around current glyph (relative to start_idx) */
	int selected_x = ((fv->glyph_idx - fv->start_idx) % fv->cols) * GRID_VIEW_CELL_W;
	int selected_y = ((fv->glyph_idx - fv->start_idx) / fv->cols) * GRID_VIEW_CELL_H;
	pxl_draw_rect(cnv, selected_x, selected_y, GRID_VIEW_CELL_W, GRID_VIEW_CELL_H);

	pxl_writer_t w;
	pxl_writer_init(&w, fv->font_family, (size_t)fv->font_family_size);
	pxl_canvas_set_color(cnv, GRID_VIEW_FG);

	for (int i = fv->start_idx; i < fv->end_idx; i++) {
		glyph_t glyph;
		if (font_view_glyph(fv, i, &glyph) != 0) continue; /* no glyph found, should not happen */

		/* Position relative to start_idx (0 = first glyph in current page) */
		int x = ((i - fv->start_idx) % fv->cols) * GRID_VIEW_CELL_W + (GRID_VIEW_CELL_W - glyph.width) / 2;
		int y = ((i - fv->start_idx) / fv->cols) * GRID_VIEW_CELL_H + (GRID_VIEW_CELL_H - glyph.height) / 2;

		pxl_writer_set_cursor(&w, x, y);
		pxl_draw_rune(cnv, &w, glyph.codepoint);
	}
}

static void
render_glyph_zoom(pxl_canvas_t *cnv, const font_view_t *fv) {
	glyph_t glyph;
	if (font_view_glyph(fv, fv->glyph_idx, &glyph) != 0) {
		/* no glyph found, should not happen */
		/* TODO: show something as error ? */
		return;
	}

	/* Calculate position relative to the glyph zoom canvas (0,0 is top-left of subview) */
	int zoom_x = (GLYPH_ZOOM_W - glyph.width * GLYPH_ZOOM_FACTOR) / 2;
	int zoom_y = (GLYPH_ZOOM_H - glyph.height * GLYPH_ZOOM_FACTOR) / 2;

	pxl_canvas_set_color(cnv, GLYPH_ZOOM_FG);
	 demo_draw_bitmask_scaled(cnv, GLYPH_ZOOM_FACTOR, glyph.bitmask, glyph.bitmask_r, zoom_x, zoom_y);
}

static void
render_glyph_characteristics(pxl_canvas_t *cnv, const font_view_t *fv) {
	glyph_t glyph;
	if (font_view_glyph(fv, fv->glyph_idx, &glyph) != 0) {
		/* no glyph found, should not happen */
		/* TODO: show something as error ? */
		return;
	}

	char text[256];
	snprintf(text, sizeof(text), "U+%04X\nW:%d  H:%d\nOff:(%+d,%+d) Adv:%d",
			(unsigned int)glyph.codepoint, glyph.width, glyph.height,
			glyph.offset_x, glyph.offset_y, glyph.advance);
	
	pxl_canvas_set_color(cnv, GLYPH_FG);

	pxl_rect_t text_bounds = pxl_str_bounds(text);
	/* Position relative to the glyph canvas (0,0 is top-left of subview) */
	int text_x = (GLYPH_W - text_bounds.w) / 16;
	int text_y = (GLYPH_H - text_bounds.h) / 2;
	pxl_draw_str(cnv, text_x, text_y, text);
}

static void
render_scrollbar(pxl_canvas_t *cnv, const font_view_t *fv) {
	if (fv->font_family_glyph_count <= fv->glyph_per_page) {
		return;
	}

	int page_h = (fv->glyph_per_page * SCROLLBAR_H) / fv->font_family_glyph_count;
	if (page_h < 8) page_h = 8;
	int page_y = (fv->start_idx * (SCROLLBAR_H - page_h)) / (fv->font_family_glyph_count - fv->glyph_per_page);

	pxl_canvas_set_color(cnv, SCROLLBAR_FG);
	/* Position relative to the scrollbar canvas (0,0 is top-left of subview) */
	pxl_fill_rect(cnv, 0, page_y, SCROLLBAR_W, page_h);
}

static void
render_footer(pxl_canvas_t *cnv, const font_view_t *fv) {
	size_t mem_size = font_view_mem_size(fv);

	char mem_size_str[32];
	fmt_mem_size(mem_size, mem_size_str, sizeof(mem_size_str));
	
	char footer_text[256];
	snprintf(footer_text, sizeof(footer_text), "Family: %d/%zu | %s | %s (%d glyphs)",
			fv->family_idx + 1, NUM_FONT_FAMILIES,
		   	fv->font_family_name,
			mem_size_str,
			fv->font_family_glyph_count);

	pxl_rect_t footer_bounds = pxl_str_bounds(footer_text);

	/* Position relative to the footer canvas (0,0 is top-left of subview) */
	int footer_x = (FOOTER_W - footer_bounds.w) / 2;
	int footer_y = (FOOTER_H - footer_bounds.h) / 2;

	pxl_canvas_set_color(cnv, FOOTER_FG);
	pxl_draw_str(cnv, footer_x, footer_y, footer_text);
}

static void
render_text_preview(pxl_canvas_t *cnv, const font_view_t *fv) {
	pxl_canvas_set_color(cnv, TEXT_PREVIEW_BG);
	pxl_canvas_clear(cnv);

	const char *text = lorem_texts[fv->family_idx];
	pxl_writer_t w;
	pxl_writer_init(&w, fv->font_family, (size_t)fv->font_family_size);
	pxl_canvas_set_color(cnv, TEXT_PREVIEW_FG);

	/* Position relative to the text preview canvas (0,0 is top-left of subview) */
	pxl_writer_set_cursor(&w, 5, 5);
	pxl_draw_text(cnv, &w, text);
}

static void
render(pxl_canvas_t *cnv, const font_view_t *fv) {
	/* Clear background */
	pxl_canvas_set_color(cnv, BG);
	pxl_canvas_clear(cnv);

	/* Setup viewports for each area */
	pxl_canvas_t cnv_title;
	pxl_canvas_set_subview(&cnv_title, cnv, TITLE_X, TITLE_Y, TITLE_W, TITLE_H);

	pxl_canvas_t cnv_grid;
	pxl_canvas_set_subview(&cnv_grid, cnv, GRID_VIEW_X, GRID_VIEW_Y, GRID_VIEW_W, GRID_VIEW_H);

	pxl_canvas_t cnv_scrollbar;
	pxl_canvas_set_subview(&cnv_scrollbar, cnv, SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_W, SCROLLBAR_H);

	pxl_canvas_t cnv_glyph_zoom;
	pxl_canvas_set_subview(&cnv_glyph_zoom, cnv, GLYPH_ZOOM_X, GLYPH_ZOOM_Y, GLYPH_ZOOM_W, GLYPH_ZOOM_H);

	pxl_canvas_t cnv_glyph;
	pxl_canvas_set_subview(&cnv_glyph, cnv, GLYPH_X, GLYPH_Y, GLYPH_W, GLYPH_H);

	pxl_canvas_t cnv_text_preview;
	pxl_canvas_set_subview(&cnv_text_preview, cnv, TEXT_PREVIEW_X, TEXT_PREVIEW_Y, TEXT_PREVIEW_W, TEXT_PREVIEW_H);

	pxl_canvas_t cnv_footer;
	pxl_canvas_set_subview(&cnv_footer, cnv, FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H);

	/* Draw title */
	pxl_canvas_set_color(&cnv_title, TITLE_BG);
	pxl_canvas_clear(&cnv_title);
	render_title(&cnv_title, fv);

	/* Draw glyph grid */
	pxl_canvas_set_color(&cnv_grid, GRID_VIEW_BG);
	pxl_canvas_clear(&cnv_grid);
	render_font_view(&cnv_grid, fv);

	/* Draw scrollbar */
	pxl_canvas_set_color(&cnv_scrollbar, SCROLLBAR_BG);
	pxl_canvas_clear(&cnv_scrollbar);
	render_scrollbar(&cnv_scrollbar, fv);

	/* Draw selected glyph bitmask */
	pxl_canvas_set_color(&cnv_glyph_zoom, GLYPH_ZOOM_BG);
	pxl_canvas_clear(&cnv_glyph_zoom);
	render_glyph_zoom(&cnv_glyph_zoom, fv);

	/* Draw selected glyph characteristics */
	pxl_canvas_set_color(&cnv_glyph, GLYPH_BG);
	pxl_canvas_clear(&cnv_glyph);
	render_glyph_characteristics(&cnv_glyph, fv);

	/* Draw text preview */
	pxl_canvas_set_color(&cnv_text_preview, TEXT_PREVIEW_BG);
	pxl_canvas_clear(&cnv_text_preview);
	render_text_preview(&cnv_text_preview, fv);

	/* Draw footer */
	pxl_canvas_set_color(&cnv_footer, FOOTER_BG);
	pxl_canvas_clear(&cnv_footer);
	render_footer(&cnv_footer, fv);
}

int
main(void) {
	pxl_app_t app = {
		.title = "PXL Font Viewer",
		.width = W,
		.height = H
		/* physics_dt defaults to 0 (no physics stepper) */
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	printf("Font Viewer.\n"
	       "Arrow/HJKL=navigate, F=switch font family.\n"
	       "ESC=quit\n");

	font_view_t fv;
	font_view_init(&fv);

	int fps = 0;

	while (pxl_app_advance_wait(&app)) {
		if (pxl_app_was_pressed(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		handle_input(&fv, &app);
		
		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);
			render(&cnv, &fv);

			/* Draw FPS in bottom right corner */
			if (fps > 0) {
				char fps_str[16];
				snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
				pxl_rect_t fps_bounds = demo_text_bounds_scaled(&font_9x15_latin, fps_str, 1);
				uint32_t fg = 0xFFFFFFFF;
				pxl_canvas_set_color(&cnv, fg);
				 demo_draw_text_scaled(&cnv, &font_9x15_latin, fps_str, 1,
					W - fps_bounds.w - 10, H - fps_bounds.h - 10);
			}

			(void)pxl_backend_end_frame();
		}
		
		 demo_update_fps(pxl_backend_get_time(), &fps);
	}

	pxl_app_deinit(&app);
	return 0;
}
