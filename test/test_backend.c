#include "backend.h"
#include "buf.h"
#include "err.h"
#include "stest/stest.h"

/* -------------------------------------------------------------------------- */
/* Note: This file tests the backend API contract and error handling.        */
/* Actual rendering is tested manually via demos (requires display server). */
/* -------------------------------------------------------------------------- */

/* Lifecycle ---------------------------------------------------------------- */

static void
test_pxl_backend_init_invalid_params(void) {
	/* Title cannot be NULL */
	ST_CHECK(pxl_backend_init(NULL, 100, 100, false) == PXL_E_INVALID_PARAM,
	         "should fail with NULL title");

	/* Width must be positive */
	ST_CHECK(pxl_backend_init("test", 0, 100, false) == PXL_E_INVALID_PARAM,
	         "should fail with w=0");
	ST_CHECK(pxl_backend_init("test", -1, 100, false) == PXL_E_INVALID_PARAM,
	         "should fail with negative w");

	/* Height must be positive */
	ST_CHECK(pxl_backend_init("test", 100, 0, false) == PXL_E_INVALID_PARAM,
	         "should fail with h=0");
	ST_CHECK(pxl_backend_init("test", 100, -1, false) == PXL_E_INVALID_PARAM,
	         "should fail with negative h");

	pxl_backend_deinit();
}

static void
test_pxl_backend_deinit_safety(void) {
	/* Deinit without prior init should not crash */
	pxl_backend_deinit();

	/* Double deinit should not crash */
	pxl_backend_init("test", 100, 100, false);
	pxl_backend_deinit();
	pxl_backend_deinit();
}



static void
test_pxl_backend_fullscreen_flag(void) {
	/* Both windowed and fullscreen should work with valid params */
	ST_CHECK(pxl_backend_init("test", 100, 100, false) == PXL_SUCCESS,
	         "windowed mode should init");
	pxl_backend_deinit();

	ST_CHECK(pxl_backend_init("test", 100, 100, true) == PXL_SUCCESS,
	         "fullscreen mode should init");
	pxl_backend_deinit();
}

static void
test_pxl_backend_get_time_basic(void) {
	ST_CHECK(pxl_backend_init("test", 100, 100, false) == PXL_SUCCESS, "init failed");

	double t1 = pxl_backend_get_time();
	/* Time should be non-negative */
	ST_CHECK(t1 >= 0.0, "time should be non-negative, got %f", t1);

	/* Time should be monotonically increasing */
	double t2 = pxl_backend_get_time();
	ST_CHECK(t2 >= t1, "time should be monotonically increasing: %f >= %f", t2, t1);

	pxl_backend_deinit();
}

static void
test_pxl_backend_poll_events_null_input(void) {
	ST_CHECK(pxl_backend_init("test", 100, 100, false) == PXL_SUCCESS, "init failed");

	/* NULL input should not crash (implementation may choose to ignore or assert) */
	pxl_input_t input_state = {0};
	pxl_backend_poll_events(&input_state);
	
	/* Also test with NULL */
	pxl_backend_poll_events(NULL);

	pxl_backend_deinit();
}

/* Frame flow -------------------------------------------------------------- */

static void
test_pxl_backend_frame_flow(void) {
	int w = 100, h = 100;
	ST_CHECK(pxl_backend_init("test", w, h, false) == PXL_SUCCESS, "init failed");

	pxl_buf_t pb;
	ST_CHECK(pxl_backend_begin_frame(&pb) == PXL_SUCCESS,
	         "begin_frame should succeed");
	
	/* out_pb should match init dimensions */
	ST_CHECK(pb.width == w, "frame width should match init width, got %d, want %d", pb.width, w);
	ST_CHECK(pb.height == h, "frame height should match init height, got %d, want %d", pb.height, h);
	ST_CHECK(pb.stride > 0, "frame stride should be positive, got %d", pb.stride);
	ST_CHECK(pb.stride % PXL_BUF_ALIGN == 0, "stride must be pixel-aligned (%d not divisible by %d)", pb.stride, PXL_BUF_ALIGN);
	ST_CHECK(pb.data != NULL, "frame data should not be NULL");

	pxl_backend_end_frame();

	pxl_backend_deinit();
}

/* Multiple frames ---------------------------------------------------------- */

static void
test_pxl_backend_multiple_frames(void) {
	ST_CHECK(pxl_backend_init("test", 100, 100, false) == PXL_SUCCESS, "init failed");

	pxl_buf_t pb;
	for (int i = 0; i < 5; i++) {
		ST_CHECK(pxl_backend_begin_frame(&pb) == PXL_SUCCESS,
		         "begin_frame %d should succeed", i);
		pxl_backend_end_frame();
	}

	pxl_backend_deinit();
}

/* Main --------------------------------------------------------------------- */

int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Lifecycle */
		ST_T(test_pxl_backend_init_invalid_params),
		ST_T(test_pxl_backend_deinit_safety),
		
		/* Frame management */
		ST_T(test_pxl_backend_frame_flow),
		ST_T(test_pxl_backend_multiple_frames),
		
		/* Options */
		ST_T(test_pxl_backend_fullscreen_flag),
		
		/* Utilities */
		ST_T(test_pxl_backend_get_time_basic),
		ST_T(test_pxl_backend_poll_events_null_input)
	);
}
