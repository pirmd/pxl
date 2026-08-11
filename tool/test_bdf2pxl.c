/* test_bdf2pxl.c - Tests for bdf2pxl */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Minimal BDF with 2 glyphs: A (65) and B (66) */
static const char minimal_bdf[] =
	"STARTFONT 2.1\n"
	"FONT minimal\n"
	"SIZE 8 72 72\n"
	"FONTBOUNDINGBOX 8 8 0 0\n"
	"STARTPROPERTIES 2\n"
	"FONT_ASCENT 8\n"
	"FONT_DESCENT 0\n"
	"ENDPROPERTIES\n"
	"CHARACTER SET\n"
	"STARTCHAR A\n"
	"ENCODING 65\n"
	"SWIDTH 500 0\n"
	"DWIDTH 8 0\n"
	"BBX 8 8 0 0\n"
	"BITMAP\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"FF\n"
	"ENDCHAR\n"
	"STARTCHAR B\n"
	"ENCODING 66\n"
	"SWIDTH 500 0\n"
	"DWIDTH 8 0\n"
	"BBX 8 8 0 0\n"
	"BITMAP\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"AA\n"
	"ENDCHAR\n"
	"ENDFONT\n";

static char *
run_command(const char *cmd, size_t *out_len) {
	FILE *fp = popen(cmd, "r");
	if (!fp) return NULL;

	size_t len = 0, cap = 4096;
	char *output = malloc(cap);
	if (!output) { pclose(fp); return NULL; }

	while (fgets(output + len, (int)(cap - len), fp)) {
		len += strlen(output + len);
		if (len >= cap - 1) {
			cap *= 2;
			char *tmp = realloc(output, cap);
			if (!tmp) { free(output); pclose(fp); return NULL; }
			output = tmp;
		}
	}
	output[len] = '\0';
	pclose(fp);
	*out_len = len;
	return output;
}

static void
write_temp_file(const char *content, size_t len, char *path, size_t path_size) {
	snprintf(path, path_size, "/tmp/bdf2pxl_test_XXXXXX");
	int fd = mkstemp(path);
	assert(fd != -1);
	write(fd, content, len);
	close(fd);
}

static void
test_normal_mode(void) {
	char bdf_path[64];
	write_temp_file(minimal_bdf, sizeof(minimal_bdf) - 1, bdf_path, sizeof(bdf_path));

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "./bdf2pxl -n test_font %s", bdf_path);

	size_t len;
	char *output = run_command(cmd, &len);
	assert(output != NULL);

	/* Check header guard */
	assert(strstr(output, "#define PXL_FONT_TEST_FONT_H") != NULL);

	/* Check includes */
	assert(strstr(output, "#include <stdint.h>") != NULL);
	assert(strstr(output, "#include \"text.h\"") != NULL);

	/* Check font definition */
	assert(strstr(output, "const pxl_font_t test_font") != NULL);

	/* Check arrays - bitmask always present, others may be NULL if all defaults */
	assert(strstr(output, "test_font_bitmask") != NULL);
	/* Widths/advances/offsets may be optimized to NULL for monospace fonts with no offsets */

	free(output);
	unlink(bdf_path);
}

static void
test_subfont_mode(void) {
	char bdf_path[64];
	write_temp_file(minimal_bdf, sizeof(minimal_bdf) - 1, bdf_path, sizeof(bdf_path));

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "./bdf2pxl -n test_font -s %s", bdf_path);

	size_t len;
	char *output = run_command(cmd, &len);
	assert(output != NULL);

	/* Must NOT contain header guard */
	assert(strstr(output, "#define PXL_FONT_TEST_FONT_H") == NULL);
	assert(strstr(output, "#ifndef") == NULL);
	assert(strstr(output, "#endif") == NULL);

	/* Must NOT contain includes */
	assert(strstr(output, "#include") == NULL);

	/* Must NOT contain source comment */
	assert(strstr(output, "Auto-generated from") == NULL);

	/* Must contain font definition, bitmask always present */
	assert(strstr(output, "test_font_bitmask") != NULL);
	assert(strstr(output, "const pxl_font_t test_font") != NULL);
	/* Widths/advances/offsets may be optimized to NULL for monospace fonts with no offsets */

	free(output);
	unlink(bdf_path);
}

static void
test_null_optimization(void) {
	char bdf_path[64];
	write_temp_file(minimal_bdf, sizeof(minimal_bdf) - 1, bdf_path, sizeof(bdf_path));

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "./bdf2pxl -n test_mono %s", bdf_path);

	size_t len;
	char *output = run_command(cmd, &len);
	assert(output != NULL);

	/* For monospace font with no offsets, all optional arrays should be NULL */
	assert(strstr(output, ".glyph_widths = NULL") != NULL);
	assert(strstr(output, ".glyph_advances = NULL") != NULL);
	assert(strstr(output, ".glyph_offsets_x = NULL") != NULL);
	assert(strstr(output, ".glyph_offsets_y = NULL") != NULL);

	/* Arrays should not be declared in output */
	assert(strstr(output, "test_mono_widths[") == NULL);
	assert(strstr(output, "test_mono_advances[") == NULL);
	assert(strstr(output, "test_mono_offsets_x[") == NULL);
	assert(strstr(output, "test_mono_offsets_y[") == NULL);

	free(output);
	unlink(bdf_path);
}

int
main(void) {
	test_normal_mode();
	test_subfont_mode();
	test_null_optimization();
	return 0;
}
