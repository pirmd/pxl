#include "test.h"
#include "backend.h"
#include "buf.h"
#include "err.h"

/* Lifecycle ---------------------------------------------------------------- */

static void
test_pxl_backend_init_invalid_params(void) {
	/* Title cannot be NULL */
	ASSERT(pxl_backend_init(NULL, 100, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	/* Width must be positive */
	ASSERT(pxl_backend_init("test", 0, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);
	ASSERT(pxl_backend_init("test", -1, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	/* Height must be positive */
	ASSERT(pxl_backend_init("test", 100, 0, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);
	ASSERT(pxl_backend_init("test", 100, -1, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	pxl_backend_deinit();
}

static void
test_pxl_backend_deinit_safety(void) {
	/* Deinit without prior init should not crash */
	pxl_backend_deinit();

	/* Double deinit should not crash */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();
	pxl_backend_deinit();
}

/* Flags -------------------------------------------------------------------- */

static void
test_pxl_backend_flags(void) {
	/* Hidden flag should work (avoids display artifacts) */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Fullscreen flag should work (even if not visible due to HIDDEN) */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Centered flag should work */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_CENTERED | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* VSYNC flag should work (may be ignored by some backends) */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_VSYNC | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Combined flags should work */
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_CENTERED | PXL_BACKEND_VSYNC | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();
}

/* Utilities ---------------------------------------------------------------- */

static void
test_pxl_backend_get_time_basic(void) {
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	double t1 = pxl_backend_get_time();
	/* Time should be non-negative */
	ASSERT(t1 >= 0.0);

	/* Time should be monotonically increasing */
	double t2 = pxl_backend_get_time();
	ASSERT(t2 >= t1);

	pxl_backend_deinit();
}

static void
test_pxl_backend_poll_events_null_input(void) {
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	/* NULL input should not crash (implementation may choose to ignore or assert) */
	pxl_input_t input_state = {0};
	pxl_backend_poll_events(&input_state);

	/* Also test with NULL */
	pxl_backend_poll_events(NULL);

	pxl_backend_deinit();
}

/* Frame flow ---------------------------------------------------------------- */

static void
test_pxl_backend_frame_flow(void) {
	int w = 100, h = 100;
	ASSERT(pxl_backend_init("test", w, h, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	pxl_buf_t pb;
	for (int i = 0; i < 5; i++) {
		ASSERT(pxl_backend_begin_frame(&pb) == PXL_SUCCESS);
		
		/* Validate dimensions on first frame only */
		if (i == 0) {
			ASSERT(pb.width == w);
			ASSERT(pb.height == h);
			ASSERT(pb.stride > 0);
			ASSERT(pb.stride % PXL_BUF_ALIGN == 0);
			ASSERT(pb.data != NULL);
		}

		pxl_backend_end_frame();
	}

	pxl_backend_deinit();
}

/* Text input ---------------------------------------------------------------- */

static void
test_pxl_backend_get_typed_text_empty(void) {
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];
	int len = pxl_backend_get_typed_text(buf, sizeof(buf));

	/* Empty buffer returns 0, but output is null-terminated */
	ASSERT(len == 0);
	ASSERT(buf[0] == '\0');

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_typed_text_edge_cases(void) {
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];

	/* out_text_max_len = 1 (only room for null terminator) */
	ASSERT(pxl_backend_get_typed_text(buf, 1) == 0);
	ASSERT(buf[0] == '\0');

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_typed_text_null_termination(void) {
	ASSERT(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];
	/* Even when empty, buffer should be null-terminated */
	pxl_backend_get_typed_text(buf, sizeof(buf));
	ASSERT(buf[0] == '\0');

	/* Poll events to clear any potential pending input */
	pxl_input_t input = {0};
	pxl_backend_poll_events(&input);

	/* Still empty, still null-terminated */
	ASSERT(pxl_backend_get_typed_text(buf, sizeof(buf)) == 0);
	ASSERT(buf[0] == '\0');

	pxl_backend_deinit();
}

/* Main --------------------------------------------------------------------- */

int
main(void) {
	test_pxl_backend_init_invalid_params();
	test_pxl_backend_deinit_safety();
	test_pxl_backend_flags();
	test_pxl_backend_get_time_basic();
	test_pxl_backend_poll_events_null_input();
	test_pxl_backend_frame_flow();
	test_pxl_backend_get_typed_text_empty();
	test_pxl_backend_get_typed_text_edge_cases();
	test_pxl_backend_get_typed_text_null_termination();
	return 0;
}
