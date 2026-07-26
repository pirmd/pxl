#include "test.h"
#include "buf.h"

#include <stddef.h>  /* for ptrdiff_t */
#include <stdlib.h>

/* pxl_calc_stride tests */

static void
test_pxl_calc_stride_exact_multiples(void) {
	ASSERT(pxl_calc_stride(0) == 0);
	ASSERT(pxl_calc_stride(4) == 4);
	ASSERT(pxl_calc_stride(8) == 8);
	ASSERT(pxl_calc_stride(100) == 100);
}

static void
test_pxl_calc_stride_rounding_up(void) {
	ASSERT(pxl_calc_stride(1) == 4);
	ASSERT(pxl_calc_stride(5) == 8);
	ASSERT(pxl_calc_stride(101) == 104);
	ASSERT(pxl_calc_stride(99) == 100);
	ASSERT(pxl_calc_stride(1000) == 1000);
	ASSERT(pxl_calc_stride(1001) == 1004);
}

static void
test_pxl_calc_stride_alignment_guarantee(void) {
	for (int w = 0; w < 1024; w++) {
		int stride = pxl_calc_stride(w);
		ASSERT(stride % PXL_BUF_ALIGN == 0);
		ASSERT(stride >= w);
	}
}

/* pxl_buf_ptr tests */

static void
test_pxl_buf_ptr_basic_access(void) {
	pxl_buf_t pb;
	pb.width = 10;
	pb.height = 10;
	pb.stride = pxl_calc_stride(10);
	pb.data = malloc(pb.stride * pb.height * sizeof(pxl_t));

	pxl_t value = 0xCAFEBABE;
	pxl_t *ptr = pxl_buf_ptr(&pb, 3, 4);
	*ptr = value;
	ASSERT(*pxl_buf_ptr(&pb, 3, 4) == value);

	free(pb.data);
}

static void
test_pxl_buf_ptr_corners(void) {
	pxl_buf_t pb;
	pb.width = 100;
	pb.height = 50;
	pb.stride = pxl_calc_stride(100);
	pb.data = malloc(pb.stride * pb.height * sizeof(pxl_t));

	/* Test all four corners */
	*pxl_buf_ptr(&pb, 0, 0) = 1;
	ASSERT(*pxl_buf_ptr(&pb, 0, 0) == 1);

	*pxl_buf_ptr(&pb, pb.width - 1, 0) = 2;
	ASSERT(*pxl_buf_ptr(&pb, pb.width - 1, 0) == 2);

	*pxl_buf_ptr(&pb, 0, pb.height - 1) = 3;
	ASSERT(*pxl_buf_ptr(&pb, 0, pb.height - 1) == 3);

	*pxl_buf_ptr(&pb, pb.width - 1, pb.height - 1) = 4;
	ASSERT(*pxl_buf_ptr(&pb, pb.width - 1, pb.height - 1) == 4);

	free(pb.data);
}

static void
test_pxl_buf_ptr_stride_calculation(void) {
	/* Test that pxl_buf_ptr correctly accounts for stride (not just width) */
	pxl_buf_t pb;
	pb.width = 101;  /* Not a multiple of PXL_BUF_ALIGN */
	pb.height = 10;
	pb.stride = pxl_calc_stride(101);  /* Will be 104 */
	pb.data = malloc(pb.stride * pb.height * sizeof(pxl_t));

	/* Verify row pointer difference equals stride */
	pxl_t *ptr_row0 = pxl_buf_ptr(&pb, 0, 0);
	pxl_t *ptr_row1 = pxl_buf_ptr(&pb, 0, 1);
	ptrdiff_t diff = (char *)ptr_row1 - (char *)ptr_row0;
	ASSERT(diff == (ptrdiff_t)(pb.stride * sizeof(pxl_t)));

	free(pb.data);
}

static void
test_pxl_buf_ptr_grid_access(void) {
	int W = 101, H = 10;
	pxl_buf_t pb;
	pb.width = W;
	pb.height = H;
	pb.stride = pxl_calc_stride(W);
	pb.data = malloc(pb.stride * pb.height * sizeof(pxl_t));

	/* Fill all pixels with unique values */
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			pxl_t *ptr = pxl_buf_ptr(&pb, x, y);
			*ptr = (pxl_t)(x + y * W);
		}
	}

	/* Verify all pixels */
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			ASSERT(*pxl_buf_ptr(&pb, x, y) == (pxl_t)(x + y * W));
		}
	}

	free(pb.data);
}

/* Main */
int
main(void) {
	test_pxl_calc_stride_exact_multiples();
	test_pxl_calc_stride_rounding_up();
	test_pxl_calc_stride_alignment_guarantee();

	test_pxl_buf_ptr_basic_access();
	test_pxl_buf_ptr_corners();
	test_pxl_buf_ptr_stride_calculation();
	test_pxl_buf_ptr_grid_access();

	return 0;
}
