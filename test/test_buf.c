#include <stdbool.h>

#include "buf.h"
#include "stest/stest.h"

/* Lifecycle --------------------------------------------------------------- */
static inline bool
is_addr_aligned(const void *addr, size_t byte_align) {
    return ((uintptr_t)addr & (byte_align - 1)) == 0;
}

static size_t
calc_stride(size_t row_sz, size_t byte_align) {
    return (row_sz + byte_align - 1) & ~(byte_align - 1);
}

static void
test_pxl_buf_init_basic(void) {
	int W = 100, H = 120;

	pxl_buf_t pb;
	ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed");
	ST_CHECK(pb.data != NULL, "pxl_buf_init returns NULL data");
	ST_CHECK(pb.width == W, "Want: %d\n Got: %d", W, pb.width);
	ST_CHECK(pb.height == H, "Want: %d\n Got: %d", H, pb.height);

	size_t want_stride = calc_stride(pb.width, PXL_ALIGN);
	ST_CHECK(pb.stride == (int)want_stride, "Want: %zu\n Got: %zu", want_stride, (size_t)pb.stride);
	ST_CHECK(is_addr_aligned(pb.data, PXL_ALIGN), "pxl_buf_init does not align data address");

	pxl_buf_deinit(&pb);
	ST_CHECK(pb.data == NULL, "pxl_buf_deinit did not set data to NULL");
}

static void
test_pxl_buf_init_invalid_dims(void) {
	pxl_buf_t pb;

	ST_CHECK(pxl_buf_init(&pb, 0, 10) == PXL_E_INVALID_PARAM, "pxl_buf_init accepts invalid param");
	ST_CHECK(pxl_buf_init(&pb, 10, 0) == PXL_E_INVALID_PARAM, "pxl_buf_init accepts invalid param");
	ST_CHECK(pxl_buf_init(&pb, 0, 0) == PXL_E_INVALID_PARAM, "pxl_buf_init accepts invalid param");
	ST_CHECK(pxl_buf_init(&pb, -1, 10) == PXL_E_INVALID_PARAM, "pxl_buf_init accepts negative width");
	ST_CHECK(pxl_buf_init(&pb, 10, -1) == PXL_E_INVALID_PARAM, "pxl_buf_init accepts negative height");
}

static void
test_pixbuf_stride_alignment(void) {
	pxl_buf_t pb;

	int W1 = 100, H1 = 10;
	if (!ST_CHECK(pxl_buf_init(&pb, W1, H1) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	size_t want_stride = calc_stride(pb.width, PXL_ALIGN);
	ST_CHECK(pb.stride == (int)want_stride, "Want: %zu\n Got: %zu", want_stride, (size_t)pb.stride);
	pxl_buf_deinit(&pb);

	int W2 = 101, H2 = 10;
	if (!ST_CHECK(pxl_buf_init(&pb, W2, H2) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	want_stride = calc_stride(pb.width, PXL_ALIGN);
	ST_CHECK(pb.stride == (int)want_stride, "Want: %zu\n Got: %zu", want_stride, (size_t)pb.stride);
	pxl_buf_deinit(&pb);
}

static void
test_pixbuf_double_free(void) {
	int W = 100, H = 10;
	pxl_buf_t pb;
	if (!ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	pxl_buf_deinit(&pb);
	pxl_buf_deinit(&pb);
	ST_CHECK(1, "double free does not crash");
}

static void
test_pxl_buf_deinit_null(void) {
	pxl_buf_t pb = { .data = NULL };
	pxl_buf_deinit(&pb);
	ST_CHECK(1, "free NULL data does not crash");
}

/* pxl_buf_ptr tests -------------------------------------------------------- */

static void
test_pxl_buf_ptr_stride(void) {
	int W = 101, H = 10;
	pxl_buf_t pb;
	if (!ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	pxl_t *ptr_row0 = pxl_buf_ptr(&pb, 0, 0);
	pxl_t *ptr_row1 = pxl_buf_ptr(&pb, 0, 1);
	ptrdiff_t diff = (char *)ptr_row1 - (char *)ptr_row0;
	ST_CHECK(diff == (ptrdiff_t)(pb.stride * sizeof(pxl_t)),
	         "row pointer difference equals stride * pixel_size");

	pxl_buf_deinit(&pb);
}

static void
test_pxl_buf_ptr_write_read(void) {
	int W = 10, H = 10;
	pxl_buf_t pb;
	if (!ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	pxl_t value = 0xCAFEBABE;
	pxl_t *ptr = pxl_buf_ptr(&pb, 3, 4);
	*ptr = value;
	ST_CHECK(*pxl_buf_ptr(&pb, 3, 4) == value,
	         "value written via pxl_buf_ptr can be read back");

	pxl_buf_deinit(&pb);
}

static void
test_pxl_buf_ptr_corners(void) {
	int W = 100, H = 50;
	pxl_buf_t pb;
	if (!ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	pxl_t *ptr;

	ptr = pxl_buf_ptr(&pb, 0, 0);
	*ptr = 1;
	ST_CHECK(*pxl_buf_ptr(&pb, 0, 0) == 1, "top-left corner works");

	ptr = pxl_buf_ptr(&pb, W - 1, 0);
	*ptr = 2;
	ST_CHECK(*pxl_buf_ptr(&pb, W - 1, 0) == 2, "top-right corner works");

	ptr = pxl_buf_ptr(&pb, 0, H - 1);
	*ptr = 3;
	ST_CHECK(*pxl_buf_ptr(&pb, 0, H - 1) == 3, "bottom-left corner works");

	ptr = pxl_buf_ptr(&pb, W - 1, H - 1);
	*ptr = 4;
	ST_CHECK(*pxl_buf_ptr(&pb, W - 1, H - 1) == 4, "bottom-right corner works");

	pxl_buf_deinit(&pb);
}

static void
test_pxl_buf_ptr_grid_access(void) {
	int W = 101, H = 10;
	pxl_buf_t pb;
	if (!ST_CHECK(pxl_buf_init(&pb, W, H) == PXL_SUCCESS, "pxl_buf_init failed")) {
		return;
	};

	size_t want_stride = calc_stride(W, PXL_ALIGN);
	ST_CHECK(pb.stride == (int)want_stride, "stride mismatch: want %zu, got %zu", want_stride, (size_t)pb.stride);

	/* Fill all pixels with unique values via pxl_buf_ptr */
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			pxl_t *ptr = pxl_buf_ptr(&pb, x, y);
			*ptr = (pxl_t)(x + y * W);
		}
	}

	/* Verify all pixels */
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			ST_CHECK(*pxl_buf_ptr(&pb, x, y) == (pxl_t)(x + y * W),
			         "pixel (%d,%d) has correct value", x, y);
		}
	}

	pxl_buf_deinit(&pb);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		ST_T(test_pxl_buf_init_basic),
		ST_T(test_pxl_buf_init_invalid_dims),
		ST_T(test_pixbuf_stride_alignment),
		ST_T(test_pixbuf_double_free),
		ST_T(test_pxl_buf_deinit_null),

		ST_T(test_pxl_buf_ptr_stride),
		ST_T(test_pxl_buf_ptr_write_read),
		ST_T(test_pxl_buf_ptr_corners),
		ST_T(test_pxl_buf_ptr_grid_access)
	);
}
