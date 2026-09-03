#include <string.h>
#include "test.h"
#include "bitmask.h"
#include "buf.h"
#include "transform.h"

/* Fixture for buffer tests */
#define BUF_W 8
#define BUF_H 8
#define BUF_STRIDE 8

static pxl_t g_buf_data[BUF_STRIDE * BUF_H];
static pxl_buf_t g_buf = {
	.data = g_buf_data,
	.width = BUF_W,
	.height = BUF_H,
	.stride = BUF_STRIDE
};

static pxl_t g_buf2_data[BUF_STRIDE * BUF_H];
static pxl_buf_t g_buf2 = {
	.data = g_buf2_data,
	.width = BUF_W,
	.height = BUF_H,
	.stride = BUF_STRIDE
};

static inline void
fixture_reset(void) {
	memset(g_buf_data, 0x00, sizeof(g_buf_data));
	memset(g_buf2_data, 0x00, sizeof(g_buf2_data));
}

/* =============================================================================
 * Buffer Flip Tests
 * =============================================================================
 */

static void
test_pxl_buf_flip_h(void) {
	fixture_reset();

	/* Fill with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)(x * 100);
		}
	}

	/* Flip horizontally */
	pxl_buf_flip_h(&g_buf2, &g_buf);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		const pxl_t *src_row = pxl_buf_ptr(&g_buf, 0, y);
		const pxl_t *dst_row = pxl_buf_ptr(&g_buf2, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			ASSERT(dst_row[x] == src_row[BUF_W - 1 - x]);
		}
	}
}

static void
test_pxl_buf_flip_v(void) {
	fixture_reset();

	/* Fill with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)(y * 100 + x);
		}
	}

	/* Flip vertically */
	pxl_buf_flip_v(&g_buf2, &g_buf);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		const pxl_t *src_row = pxl_buf_ptr(&g_buf, 0, BUF_H - 1 - y);
		const pxl_t *dst_row = pxl_buf_ptr(&g_buf2, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			ASSERT(dst_row[x] == src_row[x]);
		}
	}
}

static void
test_pxl_buf_flip_hv(void) {
	fixture_reset();

	/* Fill with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)(y * 100 + x);
		}
	}

	/* Flip both */
	pxl_buf_flip_hv(&g_buf2, &g_buf);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		const pxl_t *src_row = pxl_buf_ptr(&g_buf, 0, BUF_H - 1 - y);
		const pxl_t *dst_row = pxl_buf_ptr(&g_buf2, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			ASSERT(dst_row[x] == src_row[BUF_W - 1 - x]);
		}
	}
}

static void
test_pxl_buf_flip_h_inplace(void) {
	fixture_reset();

	/* Fill with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)(x * 100);
		}
	}

	/* Save original */
	pxl_buf_t original = g_buf;

	/* Flip in-place */
	pxl_buf_flip_h(&g_buf, &g_buf);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		const pxl_t *orig_row = pxl_buf_ptr(&original, 0, y);
		const pxl_t *dst_row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			ASSERT(dst_row[x] == orig_row[BUF_W - 1 - x]);
		}
	}
}

/* =============================================================================
 * Bitmask Flip Tests
 * =============================================================================
 */

#define BM_W 8
#define BM_H 8
#define BM_STRIDE 1

static uint8_t g_bm_data[BM_STRIDE * BM_H];
static pxl_bitmask_t g_bm = {
	.data = g_bm_data,
	.width = BM_W,
	.height = BM_H,
	.stride = BM_STRIDE
};

static uint8_t g_bm2_data[BM_STRIDE * BM_H];
static pxl_bitmask_t g_bm2 = {
	.data = g_bm2_data,
	.width = BM_W,
	.height = BM_H,
	.stride = BM_STRIDE
};

static inline void
fixture_bm_reset(void) {
	memset(g_bm_data, 0x00, sizeof(g_bm_data));
	memset(g_bm2_data, 0x00, sizeof(g_bm2_data));
}

static inline void
bm_set_bit(pxl_bitmask_t *bm, int x, int y) {
	assert(x >= 0 && x < bm->width);
	assert(y >= 0 && y < bm->height);

	size_t byte_idx = (size_t)y * bm->stride + (size_t)x / 8u;
	int bit_idx = x % 8;
	bm->data[byte_idx] |= (1U << bit_idx);
}

static inline bool
bm_get_bit(const pxl_bitmask_t *bm, int x, int y) {
	assert(x >= 0 && x < bm->width);
	assert(y >= 0 && y < bm->height);

	size_t byte_idx = (size_t)y * bm->stride + (size_t)x / 8u;
	int bit_idx = x % 8;
	return (bm->data[byte_idx] & (1U << bit_idx)) != 0;
}

static void
test_pxl_bitmask_flip_h(void) {
	fixture_bm_reset();

	/* Set some bits */
	bm_set_bit(&g_bm, 0, 0);
	bm_set_bit(&g_bm, 1, 0);
	bm_set_bit(&g_bm, 7, 7);

	/* Flip horizontally */
	pxl_bitmask_flip_h(&g_bm2, &g_bm);

	/* Check result */
	ASSERT(bm_get_bit(&g_bm2, 7, 0));  /* 0 -> 7 */
	ASSERT(bm_get_bit(&g_bm2, 6, 0));  /* 1 -> 6 */
	ASSERT(bm_get_bit(&g_bm2, 0, 7));  /* 7 -> 0 */
	ASSERT(!bm_get_bit(&g_bm2, 0, 0));
	ASSERT(!bm_get_bit(&g_bm2, 1, 0));
	ASSERT(!bm_get_bit(&g_bm2, 7, 7));
}

static void
test_pxl_bitmask_flip_v(void) {
	fixture_bm_reset();

	/* Set some bits */
	bm_set_bit(&g_bm, 0, 0);
	bm_set_bit(&g_bm, 0, 1);
	bm_set_bit(&g_bm, 7, 7);

	/* Flip vertically */
	pxl_bitmask_flip_v(&g_bm2, &g_bm);

	/* Check result */
	ASSERT(bm_get_bit(&g_bm2, 0, 7));  /* y=0 -> y=7 */
	ASSERT(bm_get_bit(&g_bm2, 0, 6));  /* y=1 -> y=6 */
	ASSERT(bm_get_bit(&g_bm2, 7, 0));  /* y=7 -> y=0 */
	ASSERT(!bm_get_bit(&g_bm2, 0, 0));
	ASSERT(!bm_get_bit(&g_bm2, 0, 1));
	ASSERT(!bm_get_bit(&g_bm2, 7, 7));
}

static void
test_pxl_bitmask_flip_hv(void) {
	fixture_bm_reset();

	/* Set some bits */
	bm_set_bit(&g_bm, 0, 0);
	bm_set_bit(&g_bm, 1, 1);
	bm_set_bit(&g_bm, 7, 7);

	/* Flip both */
	pxl_bitmask_flip_hv(&g_bm2, &g_bm);

	/* Check result */
	ASSERT(bm_get_bit(&g_bm2, 7, 7));  /* (0,0) -> (7,7) */
	ASSERT(bm_get_bit(&g_bm2, 6, 6));  /* (1,1) -> (6,6) */
	ASSERT(bm_get_bit(&g_bm2, 0, 0));  /* (7,7) -> (0,0) */
}

/* =============================================================================
 * Buffer Scale Tests
 * =============================================================================
 */

#define SCALED_W 16
#define SCALED_H 16
#define SCALED_STRIDE 16

static pxl_t g_scaled_data[SCALED_STRIDE * SCALED_H];
static pxl_buf_t g_scaled_buf = {
	.data = g_scaled_data,
	.width = SCALED_W,
	.height = SCALED_H,
	.stride = SCALED_STRIDE
};

static void
test_pxl_buf_scale_x2(void) {
	fixture_reset();

	/* Fill source with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)((y * BUF_W + x) * 100);
		}
	}

	/* Scale by 2x */
	pxl_buf_scale(&g_scaled_buf, &g_buf, 2, 2);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		for (int x = 0; x < BUF_W; x++) {
			pxl_t expected = (pxl_t)((y * BUF_W + x) * 100);

			/* Check all 4 pixels in the 2x2 block */
			for (int dy = 0; dy < 2; dy++) {
				for (int dx = 0; dx < 2; dx++) {
					pxl_t got = *pxl_buf_ptr(&g_scaled_buf, x * 2 + dx, y * 2 + dy);
					ASSERT(got == expected);
				}
			}
		}
	}
}

static void
test_pxl_buf_scale_x3(void) {
	fixture_reset();

	/* Fill source with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)((y * BUF_W + x) * 100);
		}
	}

	/* Scale by 3x in both directions */
	pxl_buf_scale(&g_scaled_buf, &g_buf, 3, 3);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		for (int x = 0; x < BUF_W; x++) {
			pxl_t expected = (pxl_t)((y * BUF_W + x) * 100);

			/* Check all 9 pixels in the 3x3 block */
			for (int dy = 0; dy < 3; dy++) {
				for (int dx = 0; dx < 3; dx++) {
					pxl_t got = *pxl_buf_ptr(&g_scaled_buf, x * 3 + dx, y * 3 + dy);
					ASSERT(got == expected);
				}
			}
		}
	}
}

static void
test_pxl_buf_scale_asymmetric(void) {
	fixture_reset();

	/* Fill source with a pattern */
	for (int y = 0; y < BUF_H; y++) {
		pxl_t *row = pxl_buf_ptr(&g_buf, 0, y);
		for (int x = 0; x < BUF_W; x++) {
			row[x] = (pxl_t)((y * BUF_W + x) * 100);
		}
	}

	/* Scale by 2x horizontally, 1x vertically */
	pxl_buf_scale(&g_scaled_buf, &g_buf, 2, 1);

	/* Check result */
	for (int y = 0; y < BUF_H; y++) {
		for (int x = 0; x < BUF_W; x++) {
			pxl_t expected = (pxl_t)((y * BUF_W + x) * 100);

			/* Check 2 pixels in the horizontal direction */
			for (int dx = 0; dx < 2; dx++) {
				pxl_t got = *pxl_buf_ptr(&g_scaled_buf, x * 2 + dx, y);
				ASSERT(got == expected);
			}
		}
	}
}

/* =============================================================================
 * Bitmask Scale Tests
 * =============================================================================
 */

#define SCALED_BM_W 16
#define SCALED_BM_H 16
#define SCALED_BM_STRIDE 2

static uint8_t g_scaled_bm_data[SCALED_BM_STRIDE * SCALED_BM_H];
static pxl_bitmask_t g_scaled_bm = {
	.data = g_scaled_bm_data,
	.width = SCALED_BM_W,
	.height = SCALED_BM_H,
	.stride = SCALED_BM_STRIDE
};

static void
test_pxl_bitmask_scale_x2(void) {
	fixture_bm_reset();

	/* Set some bits */
	bm_set_bit(&g_bm, 0, 0);
	bm_set_bit(&g_bm, 3, 3);
	bm_set_bit(&g_bm, 7, 7);

	/* Clear destination */
	memset(g_scaled_bm_data, 0x00, sizeof(g_scaled_bm_data));

	/* Scale by 2x */
	pxl_bitmask_scale(&g_scaled_bm, &g_bm, 2, 2);

	/* Check result */
	ASSERT(bm_get_bit(&g_scaled_bm, 0, 0));
	ASSERT(bm_get_bit(&g_scaled_bm, 1, 0));
	ASSERT(bm_get_bit(&g_scaled_bm, 0, 1));
	ASSERT(bm_get_bit(&g_scaled_bm, 1, 1));

	ASSERT(bm_get_bit(&g_scaled_bm, 6, 6));
	ASSERT(bm_get_bit(&g_scaled_bm, 7, 6));
	ASSERT(bm_get_bit(&g_scaled_bm, 6, 7));
	ASSERT(bm_get_bit(&g_scaled_bm, 7, 7));

	ASSERT(bm_get_bit(&g_scaled_bm, 14, 14));
	ASSERT(bm_get_bit(&g_scaled_bm, 15, 14));
	ASSERT(bm_get_bit(&g_scaled_bm, 14, 15));
	ASSERT(bm_get_bit(&g_scaled_bm, 15, 15));
}

/* =============================================================================
 * Main
 * =============================================================================
 */

int
main(void) {
	/* Buffer flip tests */
	test_pxl_buf_flip_h();
	test_pxl_buf_flip_v();
	test_pxl_buf_flip_hv();
	test_pxl_buf_flip_h_inplace();

	/* Bitmask flip tests */
	test_pxl_bitmask_flip_h();
	test_pxl_bitmask_flip_v();
	test_pxl_bitmask_flip_hv();

	/* Buffer scale tests */
	test_pxl_buf_scale_x2();
	test_pxl_buf_scale_x3();
	test_pxl_buf_scale_asymmetric();

	/* Bitmask scale tests */
	test_pxl_bitmask_scale_x2();

	return 0;
}
