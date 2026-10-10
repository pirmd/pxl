#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bitmask.h"
#include "blit.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Test fixture for transformed functions */
#define TRANSFORM_W 20
#define TRANSFORM_H 20
#define TRANSFORM_STRIDE 20

static pxl_t g_transform_buf_data[TRANSFORM_STRIDE * TRANSFORM_H];
static pxl_buf_t g_transform_buf = {
    .data = g_transform_buf_data,
    .width = TRANSFORM_W,
    .height = TRANSFORM_H,
    .stride = TRANSFORM_STRIDE
};
static pxl_canvas_t g_transform_cnv;

/* Fixture ----------------------------------------------------------------- */

#define FIXTURE_W 101
#define FIXTURE_H 128
#define FIXTURE_STRIDE 104  /* pxl_calc_stride(101) = 104 */

static pxl_t g_buf_data[FIXTURE_STRIDE * FIXTURE_H];
static pxl_buf_t g_buf = {
    .data = g_buf_data,
    .width = FIXTURE_W,
    .height = FIXTURE_H,
    .stride = FIXTURE_STRIDE
};
static pxl_canvas_t g_cnv;

static inline void
fixture_reset(void) {
    memset(g_buf_data, 0x00, sizeof(g_buf_data));
    pxl_canvas_init(&g_cnv, &g_buf);
}

/* Color constants */
#define COLOR_WHITE  0xFFFFFFFFU
#define COLOR_RED    0xFFFF0000U
#define COLOR_GREEN  0xFF00FF00U
#define COLOR_BLUE   0xFF0000FFU
#define COLOR_YELLOW 0xFFFFFF00U

/* Helpers ----------------------------------------------------------------- */

static inline bool
is_inside_scissor(int x, int y) {
    const pxl_rect_t *s = &g_cnv.scissor;
    return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

static inline bool
is_drawn_inside_rect(int x, int y, int rx, int ry, int rw, int rh) {
    rx += g_cnv.offset_x;
    ry += g_cnv.offset_y;

    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

/* Bitmask helpers ----------------------------------------------------------- */

static inline uint32_t
bitmask_get(const pxl_bitmask_t *bm, int x, int y) {
    assert(x >= 0 && x < bm->width);
    assert(y >= 0 && y < bm->height);

    size_t bit_index = (size_t)y * ((size_t)bm->stride << 3) + (size_t)x;
    size_t byte_index = bit_index >> 3;
    unsigned bit_offset = bit_index & 0x7;

    uint8_t byte = bm->data[byte_index];
    return (uint32_t)((byte >> bit_offset) & 0x1U);
}

static inline bool
is_drawn_on_bitmask(int x, int y, const pxl_bitmask_t *bm, pxl_rect_t bm_r, int cnv_x, int cnv_y) {
    cnv_x += g_cnv.offset_x;
    cnv_y += g_cnv.offset_y;

    int bm_x = bm_r.x + x - cnv_x;
    int bm_y = bm_r.y + y - cnv_y;

    if (bm_x < bm_r.x || bm_x >= bm_r.x + bm_r.w ||
            bm_y < bm_r.y || bm_y >= bm_r.y + bm_r.h) {
        return false;
    }

    return bitmask_get(bm, bm_x, bm_y) == 1;
}

/* Static source buffers for blit tests */
#define SRC_10_STRIDE 12   /* pxl_calc_stride(10) = 12 */
#define SRC_15_STRIDE 16   /* pxl_calc_stride(15) = 16 */
#define SRC_8_STRIDE 8     /* pxl_calc_stride(8) = 8 */

static pxl_t g_src_pb_10x10_data[SRC_10_STRIDE * 10];
static pxl_buf_t g_src_pb_10x10 = {
    .data = g_src_pb_10x10_data,
    .width = 10,
    .height = 10,
    .stride = SRC_10_STRIDE
};

static pxl_t g_src_pb_15x15_data[SRC_15_STRIDE * 15];
static pxl_buf_t g_src_pb_15x15 = {
    .data = g_src_pb_15x15_data,
    .width = 15,
    .height = 15,
    .stride = SRC_15_STRIDE
};

static pxl_t g_src_pb_8x8_data[SRC_8_STRIDE * 8];
static pxl_buf_t g_src_pb_8x8 = {
    .data = g_src_pb_8x8_data,
    .width = 8,
    .height = 8,
    .stride = SRC_8_STRIDE
};

static inline void
src_buf_fill(pxl_buf_t *pb, pxl_t color) {
    for (int y = 0; y < pb->height; y++) {
        pxl_t *row = pxl_buf_ptr(pb, 0, y);
        for (int x = 0; x < pb->width; x++) {
            row[x] = color;
        }
    }
}

static inline void
fixture_transform_reset(void) {
    memset(g_transform_buf_data, 0x00, sizeof(g_transform_buf_data));
    pxl_canvas_init(&g_transform_cnv, &g_transform_buf);
}

/* Static bitmasks for bitmask tests */
static uint8_t g_bm_checkerboard_data[1] = { 0xAA };
static pxl_bitmask_t g_bm_checkerboard = {
    .data = g_bm_checkerboard_data,
    .width = 8,
    .height = 2,
    .stride = 1
};

static uint8_t g_bm_all_set_16x1_data[2] = { 0xFF, 0xFF };
static pxl_bitmask_t g_bm_all_set_16x1 = {
    .data = g_bm_all_set_16x1_data,
    .width = 16,
    .height = 1,
    .stride = 2
};

static uint8_t g_bm_all_set_16x8_data[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static pxl_bitmask_t g_bm_all_set_16x8 = {
    .data = g_bm_all_set_16x8_data,
    .width = 16,
    .height = 8,
    .stride = 2
};

static uint8_t g_bm_all_set_8x1_data[1] = { 0xFF };
static pxl_bitmask_t g_bm_all_set_8x1 = {
    .data = g_bm_all_set_8x1_data,
    .width = 8,
    .height = 1,
    .stride = 1
};

/* Tests - Blit ---------------------------------------------------------------- */

static void
test_pxl_blit_rect_basic(void) {
    fixture_reset();

    pxl_t color = COLOR_GREEN;
    src_buf_fill(&g_src_pb_10x10, color);

    pxl_rect_t pb_r = {0, 0, 10, 10};
    int cnv_x = 5, cnv_y = 5;
    pxl_blit_rect(&g_cnv, &g_src_pb_10x10, pb_r, cnv_x, cnv_y);

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && in_blit ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_blit_rect_with_scissor(void) {
    fixture_reset();

    pxl_canvas_set_scissor(&g_cnv, 5, 5, 10, 10);

    pxl_t color = COLOR_GREEN;
    src_buf_fill(&g_src_pb_15x15, color);

    pxl_rect_t pb_r = {0, 0, 15, 15};
    int cnv_x = 0, cnv_y = 0;
    pxl_blit_rect(&g_cnv, &g_src_pb_15x15, pb_r, cnv_x, cnv_y);

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && in_blit ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_blit_rect_fully_clipped(void) {
    fixture_reset();

    pxl_t color = COLOR_BLUE;
    src_buf_fill(&g_src_pb_10x10, color);

    pxl_rect_t pb_r = {0, 0, 10, 10};
    int cnv_x = 30, cnv_y = 30;
    pxl_blit_rect(&g_cnv, &g_src_pb_10x10, pb_r, cnv_x, cnv_y);

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && in_blit ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_blit_rect_with_offset(void) {
    fixture_reset();

    pxl_canvas_set_offset(&g_cnv, 5, 5);

    pxl_t color = COLOR_YELLOW;
    src_buf_fill(&g_src_pb_8x8, color);

    pxl_rect_t pb_r = {0, 0, 8, 8};
    int cnv_x = 0, cnv_y = 0;
    pxl_blit_rect(&g_cnv, &g_src_pb_8x8, pb_r, cnv_x, cnv_y);

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && in_blit ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_blit_rect_partially_clipped(void) {
    fixture_reset();

    pxl_t color = COLOR_YELLOW;
    src_buf_fill(&g_src_pb_15x15, color);

    pxl_rect_t pb_r = {0, 0, 15, 15};
    int cnv_x = -5, cnv_y = -5;
    pxl_blit_rect(&g_cnv, &g_src_pb_15x15, pb_r, cnv_x, cnv_y);

    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && in_blit ? color : 0x00;
            assert(got == want);
        }
    }
}

/* Tests - Bitmask draw --------------------------------------------------------- */

static void
test_pxl_draw_bitmask_basic(void) {
    fixture_reset();

    pxl_t color = COLOR_RED;
    pxl_canvas_set_color(&g_cnv, color);

    pxl_rect_t bm_r = {0, 0, 8, 2};
    int cnv_x = 2, cnv_y = 2;
    pxl_draw_bitmask(&g_cnv, &g_bm_checkerboard, bm_r, cnv_x, cnv_y);

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_checkerboard, bm_r, cnv_x, cnv_y);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && on_bitmask ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_draw_bitmask_all_bits_set(void) {
    fixture_reset();

    pxl_t color = COLOR_GREEN;
    pxl_canvas_set_color(&g_cnv, color);

    pxl_rect_t bm_r = {0, 0, 16, 1};
    int cnv_x = 2, cnv_y = 2;
    pxl_draw_bitmask(&g_cnv, &g_bm_all_set_16x1, bm_r, cnv_x, cnv_y);

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_16x1, bm_r, cnv_x, cnv_y);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && on_bitmask ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_draw_bitmask_clipped(void) {
    fixture_reset();

    pxl_canvas_set_scissor(&g_cnv, 4, 4, 8, 8);

    pxl_t color = COLOR_BLUE;
    pxl_canvas_set_color(&g_cnv, color);

    pxl_rect_t bm_r = {0, 0, 16, 8};
    int cnv_x = 0, cnv_y = 0;
    pxl_draw_bitmask(&g_cnv, &g_bm_all_set_16x8, bm_r, cnv_x, cnv_y);

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_16x8, bm_r, cnv_x, cnv_y);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && on_bitmask ? color : 0x00;
            assert(got == want);
        }
    }
}

static void
test_pxl_draw_bitmask_with_offset(void) {
    fixture_reset();

    pxl_canvas_set_offset(&g_cnv, 5, 5);

    pxl_t color = COLOR_YELLOW;
    pxl_canvas_set_color(&g_cnv, color);

    pxl_rect_t bm_r = {0, 0, 8, 1};
    int cnv_x = 0, cnv_y = 0;
    pxl_draw_bitmask(&g_cnv, &g_bm_all_set_8x1, bm_r, cnv_x, cnv_y);

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            bool in_s = is_inside_scissor(x, y);
            bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_8x1, bm_r, cnv_x, cnv_y);

            pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
            pxl_t want = in_s && on_bitmask ? color : 0x00;
            assert(got == want);
        }
    }
}

/* Tests for transformed bitmask drawing */

static void
test_pxl_draw_bitmask_transformed_scale2(void) {
    fixture_transform_reset();

    pxl_t color = COLOR_GREEN;
    pxl_canvas_set_color(&g_transform_cnv, color);

    /* Create a 4x4 bitmask with all bits set */
    uint8_t bm_data[2] = { 0xFF, 0xFF };
    pxl_bitmask_t bm = { .data = bm_data, .width = 4, .height = 2, .stride = 1 };

    pxl_rect_t bm_r = {0, 0, 4, 2};
    int x = 5, y = 5;
    pxl_draw_bitmask_transformed(&g_transform_cnv, &bm, bm_r, x, y, 2, PXL_FLIP_NONE);

    /* Check that a 8x4 rectangle is drawn */
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            int px = x + dx;
            int py = y + dy;
            pxl_t got = *pxl_buf_ptr(&g_transform_buf, px, py);
            assert(got == color);
        }
    }
}

static void
test_pxl_draw_bitmask_transformed_flip_h(void) {
    fixture_transform_reset();

    pxl_t color = COLOR_RED;
    pxl_canvas_set_color(&g_transform_cnv, color);

    /* Create a bitmask with only bit 0 set (leftmost pixel) */
    uint8_t bm_data[1] = { 0x01 };
    pxl_bitmask_t bm = { .data = bm_data, .width = 4, .height = 1, .stride = 1 };

    pxl_rect_t bm_r = {0, 0, 4, 1};
    int x = 5, y = 5;
    pxl_draw_bitmask_transformed(&g_transform_cnv, &bm, bm_r, x, y, 1, PXL_FLIP_H);

    /* With flip H, bit 0 (leftmost source) appears at rightmost destination */
    pxl_t got = *pxl_buf_ptr(&g_transform_buf, 5 + 3, 5);
    assert(got == color); /* Rightmost pixel should be on (source bit 0) */
    got = *pxl_buf_ptr(&g_transform_buf, 5, 5);
    assert(got == 0); /* Leftmost pixel should be off */
}

static void
test_pxl_draw_bitmask_transformed_flip_v(void) {
    fixture_transform_reset();

    pxl_t color = COLOR_BLUE;
    pxl_canvas_set_color(&g_transform_cnv, color);

    /* Create a 1x2 bitmask with top bit set and bottom bit clear */
    uint8_t bm_data[2] = { 0x01, 0x00 }; /* First row has bit 0 set, second row has bit 0 clear */
    pxl_bitmask_t bm = { .data = bm_data, .width = 1, .height = 2, .stride = 1 };

    pxl_rect_t bm_r = {0, 0, 1, 2};
    int x = 5, y = 5;
    pxl_draw_bitmask_transformed(&g_transform_cnv, &bm, bm_r, x, y, 1, PXL_FLIP_V);

    /* With flip V, the top source bit appears at bottom destination */
    pxl_t got = *pxl_buf_ptr(&g_transform_buf, 5, 5 + 1);
    assert(got == color); /* Bottom pixel should be on (source top bit) */
    got = *pxl_buf_ptr(&g_transform_buf, 5, 5);
    assert(got == 0); /* Top pixel should be off (source bottom bit was off) */
}

static void
test_pxl_blit_transformed_scale2(void) {
    fixture_transform_reset();

    /* Fill source buffer with a pattern */
    pxl_t color = COLOR_YELLOW;
    src_buf_fill(&g_src_pb_8x8, color);

    pxl_rect_t pb_r = {0, 0, 4, 4};
    int x = 5, y = 5;
    pxl_blit_transformed(&g_transform_cnv, &g_src_pb_8x8, pb_r, x, y, 2, PXL_FLIP_NONE);

    /* Check that a 8x8 rectangle is drawn */
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            int px = x + dx;
            int py = y + dy;
            pxl_t got = *pxl_buf_ptr(&g_transform_buf, px, py);
            assert(got == color);
        }
    }
}

static void
test_pxl_blit_transformed_flip_h(void) {
    fixture_transform_reset();

    /* Fill source buffer with a pattern */
    for (int y = 0; y < 8; y++) {
        pxl_t *row = pxl_buf_ptr(&g_src_pb_8x8, 0, y);
        for (int x = 0; x < 8; x++) {
            row[x] = (pxl_t)(x * 100);
        }
    }

    pxl_rect_t pb_r = {0, 0, 8, 8};
    int x = 2, y = 2;
    pxl_blit_transformed(&g_transform_cnv, &g_src_pb_8x8, pb_r, x, y, 1, PXL_FLIP_H);

    /* With flip H, source[src_x] is drawn at dst_x + (width-1-src_x)
     * so at destination position (x + (7-src_x)), we have source[src_x] */
    for (int src_y = 0; src_y < 8; src_y++) {
        for (int src_x = 0; src_x < 8; src_x++) {
            int px = x + (7 - src_x); /* flipped destination position */
            int py = y + src_y;
            pxl_t got = *pxl_buf_ptr(&g_transform_buf, px, py);
            pxl_t expected = (pxl_t)(src_x * 100); /* source[src_x] */
            assert(got == expected);
        }
    }
}

static void
test_pxl_blit_transformed_flip_v(void) {
    fixture_transform_reset();

    /* Fill source buffer with a pattern */
    for (int y = 0; y < 8; y++) {
        pxl_t *row = pxl_buf_ptr(&g_src_pb_8x8, 0, y);
        for (int x = 0; x < 8; x++) {
            row[x] = (pxl_t)(y * 100);
        }
    }

    pxl_rect_t pb_r = {0, 0, 8, 8};
    int x = 2, y = 2;
    pxl_blit_transformed(&g_transform_cnv, &g_src_pb_8x8, pb_r, x, y, 1, PXL_FLIP_V);

    /* With flip V, source[src_y] is drawn at dst_y + (height-1-src_y) */
    for (int src_y = 0; src_y < 8; src_y++) {
        for (int src_x = 0; src_x < 8; src_x++) {
            int px = x + src_x;
            int py = y + (7 - src_y); /* flipped destination position */
            pxl_t got = *pxl_buf_ptr(&g_transform_buf, px, py);
            pxl_t expected = (pxl_t)(src_y * 100); /* source[src_y] */
            assert(got == expected);
        }
    }
}

static void
test_pxl_blit_transformed_scale_and_flip(void) {
    fixture_transform_reset();

    /* Fill source buffer with a pattern */
    for (int y = 0; y < 4; y++) {
        pxl_t *row = pxl_buf_ptr(&g_src_pb_8x8, 0, y);
        for (int x = 0; x < 4; x++) {
            row[x] = (pxl_t)(y * 10 + x);
        }
    }

    pxl_rect_t pb_r = {0, 0, 4, 4};
    int x = 5, y = 5;
    pxl_blit_transformed(&g_transform_cnv, &g_src_pb_8x8, pb_r, x, y, 2, PXL_FLIP_H);

    /* Check that pixels are scaled by 2 and flipped horizontally
     * source[sx,sy] is drawn at x + (3-sx)*2, y + sy*2 */
    for (int sy = 0; sy < 4; sy++) {
        for (int sx = 0; sx < 4; sx++) {
            pxl_t expected = (pxl_t)(sy * 10 + sx); /* source[sx,sy] */
            /* Each source pixel becomes a 2x2 block at flipped position */
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    int px = x + (3 - sx) * 2 + dx; /* Flipped destination position */
                    int py = y + sy * 2 + dy;
                    pxl_t got = *pxl_buf_ptr(&g_transform_buf, px, py);
                    assert(got == expected);
                }
            }
        }
    }
}

/* Example tests from documentation */

static void
test_example_pxl_draw_bitmask_transformed(void) {
    /* Example: Draw a bitmask with 2x scaling */
    pxl_buf_t pb = { .data = g_transform_buf_data, .width = TRANSFORM_W, .height = TRANSFORM_H, .stride = TRANSFORM_STRIDE };
    pxl_canvas_t cnv;
    pxl_canvas_init(&cnv, &pb);
    memset(g_transform_buf_data, 0x00, sizeof(g_transform_buf_data));

    uint8_t bm_data[1] = { 0xFF };
    pxl_bitmask_t bm = { .data = bm_data, .width = 8, .height = 1, .stride = 1 };

    pxl_canvas_set_color(&cnv, 0xFFFFFFFF);
    pxl_draw_bitmask_transformed(&cnv, &bm, (pxl_rect_t){0, 0, 8, 1}, 0, 0, 2, PXL_FLIP_NONE);

    /* Verify: 16x2 rectangle should be drawn */
    assert(*pxl_buf_ptr(&pb, 0, 0) == 0xFFFFFFFF);
    assert(*pxl_buf_ptr(&pb, 15, 1) == 0xFFFFFFFF);
}

static void
test_example_pxl_blit_transformed(void) {
    /* Example: Blit with 2x scaling */
    pxl_buf_t pb = { .data = g_transform_buf_data, .width = TRANSFORM_W, .height = TRANSFORM_H, .stride = TRANSFORM_STRIDE };
    pxl_canvas_t cnv;
    pxl_canvas_init(&cnv, &pb);
    memset(g_transform_buf_data, 0x00, sizeof(g_transform_buf_data));

    pxl_t src_data[4 * 4];
    pxl_buf_t src = { .data = src_data, .width = 4, .height = 4, .stride = 4 };
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            src_data[y * 4 + x] = 0xFF000000U | (uint32_t)(x * 64 + y * 16);
        }
    }

    pxl_blit_transformed(&cnv, &src, (pxl_rect_t){0, 0, 4, 4}, 0, 0, 2, PXL_FLIP_NONE);

    /* Verify: 8x8 rectangle should be drawn with scaled pixels */
    assert(*pxl_buf_ptr(&pb, 0, 0) == 0xFF000000);
    assert(*pxl_buf_ptr(&pb, 7, 7) == (0xFF000000 | (3 * 64 + 3 * 16)));
}

/* Main ----------------------------------------------------------------------- */
int
main(void) {
    /* Blit tests */
    test_pxl_blit_rect_basic();
    test_pxl_blit_rect_with_scissor();
    test_pxl_blit_rect_fully_clipped();
    test_pxl_blit_rect_with_offset();
    test_pxl_blit_rect_partially_clipped();

    /* Bitmask draw tests */
    test_pxl_draw_bitmask_basic();
    test_pxl_draw_bitmask_all_bits_set();
    test_pxl_draw_bitmask_clipped();
    test_pxl_draw_bitmask_with_offset();

    /* Transformed bitmask draw tests */
    test_pxl_draw_bitmask_transformed_scale2();
    test_pxl_draw_bitmask_transformed_flip_h();
    test_pxl_draw_bitmask_transformed_flip_v();

    /* Transformed blit tests */
    test_pxl_blit_transformed_scale2();
    test_pxl_blit_transformed_flip_h();
    test_pxl_blit_transformed_flip_v();
    test_pxl_blit_transformed_scale_and_flip();

    /* Example tests */
    test_example_pxl_draw_bitmask_transformed();
    test_example_pxl_blit_transformed();

    return 0;
}
