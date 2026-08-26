#include <string.h>
#include "buf.h"
#include "canvas.h"
#include "tileset.h"
#include "test.h"

/* Constants --------------------------------------------------------------- */

#define ATLAS_COLS   4
#define ATLAS_ROWS   4
#define TILE_W       8
#define TILE_H       8
#define ATLAS_W      (ATLAS_COLS * TILE_W)
#define ATLAS_H      (ATLAS_ROWS * TILE_H)
#define ATLAS_STRIDE 32  /* pxl_calc_stride(32) = 32 */

#define PB_W        32
#define PB_H        32
#define PB_STRIDE    32  /* pxl_calc_stride(32) = 32 */

/* Static buffers ---------------------------------------------------------- */

/* Atlas buffer: contains colored tiles */
static pxl_t g_atlas_data[ATLAS_STRIDE * ATLAS_H];
static pxl_buf_t g_atlas = {
	.data = g_atlas_data,
	.width = ATLAS_W,
	.height = ATLAS_H,
	.stride = ATLAS_STRIDE
};

/* Pixel buffer for drawing */
static pxl_t g_pb_data[PB_STRIDE * PB_H];
static pxl_buf_t g_pb = {
	.data = g_pb_data,
	.width = PB_W,
	.height = PB_H,
	.stride = PB_STRIDE
};

static pxl_canvas_t g_cnv;
static pxl_tileset_t g_ts;

/* Fixture ---------------------------------------------------------------- */

static void
fixture_setup(void) {
	/* Initialize atlas with colored tiles (each tile has unique color). */
	for (int r = 0; r < ATLAS_ROWS; r++) {
		for (int c = 0; c < ATLAS_COLS; c++) {
			int tile_idx = r * ATLAS_COLS + c;
			uint8_t r_val = (uint8_t)((tile_idx * 17) % 256);
			uint8_t g_val = (uint8_t)((tile_idx * 31) % 256);
			uint8_t b_val = (uint8_t)((tile_idx * 59) % 256);
			pxl_t color = (pxl_t)(0xFF000000U | (r_val << 16) | (g_val << 8) | b_val);

			for (int ty = 0; ty < TILE_H; ty++) {
				pxl_t *dst_row = pxl_buf_ptr(&g_atlas, c * TILE_W, r * TILE_H + ty);
				for (int tx = 0; tx < TILE_W; tx++) {
					dst_row[tx] = color;
				}
			}
		}
	}

	g_ts = (pxl_tileset_t){&g_atlas, TILE_W, TILE_H, ATLAS_COLS, ATLAS_ROWS};
	pxl_canvas_init(&g_cnv, &g_pb);
}

static inline void
fixture_reset(void) {
	memset(g_pb_data, 0x00, sizeof(g_pb_data));
}

/* Helpers ---------------------------------------------------------------- */

static bool
has_pixels_in_rect(pxl_rect_t rect) {
	for (int y = rect.y; y < rect.y + rect.h; y++) {
		for (int x = rect.x; x < rect.x + rect.w; x++) {
			if (*pxl_buf_ptr(&g_pb, x, y) != 0) return true;
		}
	}
	return false;
}

/* Tests ----------------------------------------------------------------- */

static void
test_pxl_draw_tile_basic(void) {
	fixture_reset();

	/* Draw tile 0 at (5, 5) */
	pxl_draw_tile(&g_cnv, &g_ts, 0, 5, 5);

	/* Verify pixels were drawn in expected area */
	ASSERT(has_pixels_in_rect((pxl_rect_t){5, 5, 8, 8}));

	/* Verify no pixels outside the tile area were drawn */
	ASSERT(!has_pixels_in_rect((pxl_rect_t){0, 0, 5, 32}));
	ASSERT(!has_pixels_in_rect((pxl_rect_t){0, 0, 32, 5}));
	ASSERT(!has_pixels_in_rect((pxl_rect_t){13, 0, 19, 32}));
}

static void
test_pxl_draw_tile_multiple(void) {
	fixture_reset();

	/* Draw multiple tiles at different positions */
	pxl_draw_tile(&g_cnv, &g_ts, 0, 0, 0);
	pxl_draw_tile(&g_cnv, &g_ts, 1, 10, 0);
	pxl_draw_tile(&g_cnv, &g_ts, 4, 0, 10);

	/* Verify all tiles were drawn */
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	ASSERT(has_pixels_in_rect((pxl_rect_t){10, 0, 8, 8}));
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 10, 8, 8}));
}

static void
test_pxl_draw_tile_with_offset(void) {
	fixture_reset();

	/* Set canvas offset */
	pxl_canvas_set_offset(&g_cnv, 5, 5);

	/* Draw tile 0 at (0, 0) - with offset becomes (5, 5) */
	pxl_draw_tile(&g_cnv, &g_ts, 0, 0, 0);

	/* Verify tile was drawn at offset position */
	ASSERT(has_pixels_in_rect((pxl_rect_t){5, 5, 8, 8}));

	/* Verify no pixels at (0,0) */
	ASSERT(!has_pixels_in_rect((pxl_rect_t){0, 0, 5, 5}));

	pxl_canvas_reset_offset(&g_cnv);
}

static void
test_pxl_draw_tile_with_scissor(void) {
	fixture_reset();

	/* Set scissor */
	pxl_canvas_set_scissor(&g_cnv, 8, 8, 16, 16);

	/* Draw tile 0 at (5, 5) - partially clipped by scissor */
	pxl_draw_tile(&g_cnv, &g_ts, 0, 5, 5);

	/* Verify pixels inside scissor were drawn */
	ASSERT(has_pixels_in_rect((pxl_rect_t){8, 8, 8, 8}));

	/* Verify pixels outside scissor were NOT drawn */
	ASSERT(!has_pixels_in_rect((pxl_rect_t){5, 5, 3, 3}));
	ASSERT(!has_pixels_in_rect((pxl_rect_t){13, 13, 8, 8}));

	pxl_canvas_reset_scissor(&g_cnv);
}

static void
test_pxl_draw_sprite_sequential(void) {
	fixture_reset();

	/* Create sprite with sequential frames: tiles 0,1,2 */
	pxl_sprite_t sprite = {0, 3, NULL};

	/* Draw frame 0 (tile 0) */
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 0, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color0 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Draw frame 1 (tile 1) */
	fixture_reset();
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 1, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color1 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Draw frame 2 (tile 2) */
	fixture_reset();
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 2, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color2 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Verify all frames have different colors */
	ASSERT(color0 != color1);
	ASSERT(color0 != color2);
	ASSERT(color1 != color2);
}

static void
test_pxl_draw_sprite_custom_frames(void) {
	fixture_reset();

	/* Create sprite with custom frames: [0, 4, 8] */
	uint16_t custom_frames[] = {0, 4, 8};
	pxl_sprite_t sprite = {0, 3, custom_frames};

	/* Draw frame 0 (tile 0) */
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 0, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color0 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Draw frame 1 (tile 4) */
	fixture_reset();
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 1, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color1 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Draw frame 2 (tile 8) */
	fixture_reset();
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 2, 0, 0);
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));
	pxl_t color2 = *pxl_buf_ptr(&g_pb, 0, 0);

	/* Verify all frames have different colors */
	ASSERT(color0 != color1);
	ASSERT(color0 != color2);
	ASSERT(color1 != color2);
}

static void
test_pxl_draw_tile_last_tile(void) {
	/* Test drawing the last tile in the grid (at cols*rows - 1).
	 * This verifies that grid boundary calculations are correct.
	 */
	fixture_reset();

	int last_tile_idx = g_ts.cols * g_ts.rows - 1;  // = 15
	pxl_draw_tile(&g_cnv, &g_ts, last_tile_idx, 0, 0);

	/* Verify last tile was drawn at expected position */
	ASSERT(has_pixels_in_rect((pxl_rect_t){0, 0, 8, 8}));

	/* Verify it has the expected color (tile 15's color from the atlas) */
	pxl_t color = *pxl_buf_ptr(&g_pb, 0, 0);
	uint8_t expected_r = (uint8_t)((15 * 17) % 256);
	uint8_t expected_g = (uint8_t)((15 * 31) % 256);
	uint8_t expected_b = (uint8_t)((15 * 59) % 256);
	pxl_t expected_color = (pxl_t)(0xFF000000U | (expected_r << 16) | (expected_g << 8) | expected_b);
	ASSERT(color == expected_color);
}

static void
test_pxl_draw_tile_with_offset_and_scissor(void) {
	fixture_reset();

	pxl_sprite_t sprite;
	uint16_t custom[] = {0};
	sprite = (pxl_sprite_t){0, 1, custom};

	/* Setup canvas with offset and scissor */
	pxl_canvas_set_offset(&g_cnv, 2, 2);
	pxl_canvas_set_scissor(&g_cnv, 4, 4, 16, 16);

	/* Draw sprite at (0, 0) - with offset becomes (2, 2), clipped by scissor (4,4,16,16) */
	pxl_draw_sprite(&g_cnv, &g_ts, &sprite, 0, 0, 0);

	/* Verify pixels at offset+scissor intersection were drawn */
	ASSERT(has_pixels_in_rect((pxl_rect_t){4, 4, 8, 8}));

	/* Verify pixels before offset+scissor were NOT drawn */
	ASSERT(!has_pixels_in_rect((pxl_rect_t){2, 2, 2, 2}));

	pxl_canvas_reset_offset(&g_cnv);
	pxl_canvas_reset_scissor(&g_cnv);
}

/* Main --------------------------------------------------------------------- */

int
main(void) {
	fixture_setup();

	test_pxl_draw_tile_basic();
	test_pxl_draw_tile_multiple();
	test_pxl_draw_tile_with_offset();
	test_pxl_draw_tile_with_scissor();
	test_pxl_draw_tile_last_tile();
	test_pxl_draw_sprite_sequential();
	test_pxl_draw_sprite_custom_frames();
	test_pxl_draw_tile_with_offset_and_scissor();

	return 0;
}
