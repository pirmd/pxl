#include <string.h>
#include "buf.h"
#include "canvas.h"
#include "tileset.h"
#include "stest/stest.h"

/* Fixture ----------------------------------------------------------------- */

typedef struct {
	pxl_buf_t atlas;
	pxl_buf_t pb;
	pxl_canvas_t cnv;
	pxl_tileset_t ts;
} fixture_t;

static void
pxl_buf_zero(pxl_buf_t *pb) {
	assert(pb && pb->data);
	memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pxl_t));
}

/* Initialize a test atlas with colored tiles */
static bool
fixture_init_atlas(fixture_t *f, int tile_w, int tile_h, int cols, int rows) {
	int atlas_w = cols * tile_w;
	int atlas_h = rows * tile_h;

	f->atlas.width = atlas_w;
	f->atlas.height = atlas_h;
	f->atlas.stride = pxl_calc_stride(atlas_w);
	f->atlas.data = malloc(f->atlas.stride * f->atlas.height * sizeof(pxl_t));
	if (f->atlas.data == NULL) {
		return false;
	}

	/* Fill atlas with colored tiles (each tile has unique color). */
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			int tile_idx = r * cols + c;
			uint8_t r_val = (uint8_t)((tile_idx * 17) % 256);
			uint8_t g_val = (uint8_t)((tile_idx * 31) % 256);
			uint8_t b_val = (uint8_t)((tile_idx * 59) % 256);
			pxl_t color = (pxl_t)(0xFF000000U | (r_val << 16) | (g_val << 8) | b_val);

			for (int ty = 0; ty < tile_h; ty++) {
				pxl_t *dst_row = pxl_buf_ptr(&f->atlas, c * tile_w, r * tile_h + ty);
				for (int tx = 0; tx < tile_w; tx++) {
					dst_row[tx] = color;
				}
			}
		}
	}

	f->ts = (pxl_tileset_t){&f->atlas, tile_w, tile_h, cols};
	return true;
}

static bool
fixture_init(const st_ctx_t *ctx, fixture_t *f, int w, int h, int tile_w, int tile_h, int cols, int rows) {
	if (!st_check(ctx, fixture_init_atlas(f, tile_w, tile_h, cols, rows),
	              "Failed to initialize atlas")) {
		return false;
	}
	f->pb.width = w;
	f->pb.height = h;
	f->pb.stride = pxl_calc_stride(w);
	f->pb.data = malloc(f->pb.stride * f->pb.height * sizeof(pxl_t));
	if (!st_check(ctx, f->pb.data != NULL, "malloc failed")) {
		return false;
	}
	pxl_canvas_init(&f->cnv, &f->pb);
	pxl_buf_zero(&f->pb);
	return true;
}

static void
fixture_deinit(fixture_t *f) {
	free(f->atlas.data);
	f->atlas.data = NULL;
	free(f->pb.data);
	f->pb.data = NULL;
}

/* Helpers ----------------------------------------------------------------- */

static inline bool
has_pixels_in_rect(const pxl_buf_t *pb, pxl_rect_t rect) {
	for (int y = rect.y; y < rect.y + rect.h; y++) {
		for (int x = rect.x; x < rect.x + rect.w; x++) {
			if (*pxl_buf_ptr(pb, x, y) != 0) return true;
		}
	}
	return false;
}

/* Tests ----------------------------------------------------------------- */

static void
test_pxl_draw_tile_basic(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Draw tile 0 at (5, 5) */
	pxl_draw_tile(&f.cnv, &f.ts, 0, 5, 5);

	/* Verify pixels were drawn in expected area */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){5, 5, 8, 8}),
	         "No pixels drawn for tile 0 at (5,5)");

	/* Verify no pixels outside the tile area were drawn */
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 5, 32}),
	         "Pixels drawn outside tile area (left)");
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 32, 5}),
	         "Pixels drawn outside tile area (top)");
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){13, 0, 19, 32}),
	         "Pixels drawn outside tile area (right)");

	fixture_deinit(&f);
}

static void
test_pxl_draw_tile_multiple(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Draw multiple tiles at different positions */
	pxl_draw_tile(&f.cnv, &f.ts, 0, 0, 0);
	pxl_draw_tile(&f.cnv, &f.ts, 1, 10, 0);
	pxl_draw_tile(&f.cnv, &f.ts, 4, 0, 10);

	/* Verify all tiles were drawn */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for tile 0");
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){10, 0, 8, 8}),
	         "No pixels drawn for tile 1");
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 10, 8, 8}),
	         "No pixels drawn for tile 4");

	fixture_deinit(&f);
}

static void
test_pxl_draw_tile_with_offset(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Set canvas offset */
	pxl_canvas_set_offset(&f.cnv, 5, 5);

	/* Draw tile 0 at (0, 0) - with offset becomes (5, 5) */
	pxl_draw_tile(&f.cnv, &f.ts, 0, 0, 0);

	/* Verify tile was drawn at offset position */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){5, 5, 8, 8}),
	         "Tile not drawn at offset position");

	/* Verify no pixels at (0,0) */
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 5, 5}),
	         "Pixels drawn at (0,0) instead of offset position");

	pxl_canvas_reset_offset(&f.cnv);
	fixture_deinit(&f);
}

static void
test_pxl_draw_tile_with_scissor(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Set scissor */
	pxl_canvas_set_scissor(&f.cnv, 8, 8, 16, 16);

	/* Draw tile 0 at (5, 5) - partially clipped by scissor */
	pxl_draw_tile(&f.cnv, &f.ts, 0, 5, 5);

	/* Verify pixels inside scissor were drawn */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){8, 8, 8, 8}),
	         "No pixels drawn inside scissor area");

	/* Verify pixels outside scissor were NOT drawn */
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){5, 5, 3, 3}),
	         "Pixels drawn outside scissor area (top-left)");
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){13, 13, 8, 8}),
	         "Pixels drawn outside scissor area (bottom-right)");

	pxl_canvas_reset_scissor(&f.cnv);
	fixture_deinit(&f);
}

static void
test_pxl_draw_sprite_sequential(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Create sprite with sequential frames: tiles 0,1,2 */
	pxl_sprite_t sprite = {0, 3, NULL};

	/* Draw frame 0 (tile 0) */
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 0, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for sprite frame 0");
	pxl_t color0 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Draw frame 1 (tile 1) */
	pxl_buf_zero(&f.pb);
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 1, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for sprite frame 1");
	pxl_t color1 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Draw frame 2 (tile 2) */
	pxl_buf_zero(&f.pb);
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 2, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for sprite frame 2");
	pxl_t color2 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Verify all frames have different colors */
	ST_CHECK(color0 != color1, "Frame 0 and 1 have same color");
	ST_CHECK(color0 != color2, "Frame 0 and 2 have same color");
	ST_CHECK(color1 != color2, "Frame 1 and 2 have same color");

	fixture_deinit(&f);
}

static void
test_pxl_draw_sprite_custom_frames(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	/* Create sprite with custom frames: [0, 4, 8] */
	uint16_t custom_frames[] = {0, 4, 8};
	pxl_sprite_t sprite = {0, 3, custom_frames};

	/* Draw frame 0 (tile 0) */
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 0, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for custom frame 0");
	pxl_t color0 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Draw frame 1 (tile 4) */
	pxl_buf_zero(&f.pb);
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 1, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for custom frame 1");
	pxl_t color1 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Draw frame 2 (tile 8) */
	pxl_buf_zero(&f.pb);
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 2, 0, 0);
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){0, 0, 8, 8}),
	         "No pixels drawn for custom frame 2");
	pxl_t color2 = *pxl_buf_ptr(&f.pb, 0, 0);

	/* Verify all frames have different colors */
	ST_CHECK(color0 != color1, "Custom frame 0 and 1 have same color");
	ST_CHECK(color0 != color2, "Custom frame 0 and 2 have same color");
	ST_CHECK(color1 != color2, "Custom frame 1 and 2 have same color");

	fixture_deinit(&f);
}

static void
test_pxl_sprite_get_tile_sequential(void) {
	pxl_sprite_t sequential = {10, 3, NULL};
	ST_CHECK(pxl_sprite_get_tile(&sequential, 0) == 10,
	         "Sequential sprite: frame 0 should be tile 10");
	ST_CHECK(pxl_sprite_get_tile(&sequential, 1) == 11,
	         "Sequential sprite: frame 1 should be tile 11");
	ST_CHECK(pxl_sprite_get_tile(&sequential, 2) == 12,
	         "Sequential sprite: frame 2 should be tile 12");
}

static void
test_pxl_sprite_get_tile_custom(void) {
	uint16_t custom[] = {5, 15, 25};
	pxl_sprite_t custom_sprite = {0, 3, custom};
	ST_CHECK(pxl_sprite_get_tile(&custom_sprite, 0) == 5,
	         "Custom sprite: frame 0 should be tile 5");
	ST_CHECK(pxl_sprite_get_tile(&custom_sprite, 1) == 15,
	         "Custom sprite: frame 1 should be tile 15");
	ST_CHECK(pxl_sprite_get_tile(&custom_sprite, 2) == 25,
	         "Custom sprite: frame 2 should be tile 25");
}

static void
test_pxl_draw_tile_with_offset_and_scissor(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 32, 32, 8, 8, 4, 4)) return;

	pxl_sprite_t sprite;
	uint16_t custom[] = {0};
	sprite = (pxl_sprite_t){0, 1, custom};

	/* Setup canvas with offset and scissor */
	pxl_canvas_set_offset(&f.cnv, 2, 2);
	pxl_canvas_set_scissor(&f.cnv, 4, 4, 16, 16);

	/* Draw sprite at (0, 0) - with offset becomes (2, 2), clipped by scissor (4,4,16,16) */
	pxl_draw_sprite(&f.cnv, &f.ts, &sprite, 0, 0, 0);

	/* Verify pixels at offset+scissor intersection were drawn */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){4, 4, 8, 8}),
	         "No pixels drawn in scissor area with offset");

	/* Verify pixels before offset+scissor were NOT drawn */
	ST_CHECK(!has_pixels_in_rect(&f.pb, (pxl_rect_t){2, 2, 2, 2}),
	         "Pixels drawn before scissor area");

	pxl_canvas_reset_offset(&f.cnv);
	pxl_canvas_reset_scissor(&f.cnv);
	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* pxl_draw_tile tests */
		ST_T(test_pxl_draw_tile_basic),
		ST_T(test_pxl_draw_tile_multiple),
		ST_T(test_pxl_draw_tile_with_offset),
		ST_T(test_pxl_draw_tile_with_scissor),
		/* pxl_draw_sprite tests */
		ST_T(test_pxl_draw_sprite_sequential),
		ST_T(test_pxl_draw_sprite_custom_frames),
		/* pxl_sprite_get_tile tests */
		ST_T(test_pxl_sprite_get_tile_sequential),
		ST_T(test_pxl_sprite_get_tile_custom),
		/* Combined tests */
		ST_T(test_pxl_draw_tile_with_offset_and_scissor)
	);
}
