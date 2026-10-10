#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "backend.h"
#include "buf.h"
#include "err.h"
#include "input.h"
#include "text.h"

/* Lifecycle ---------------------------------------------------------------- */

static void
test_pxl_backend_init_invalid_params(void) {
	/* Title cannot be NULL */
	assert(pxl_backend_init(NULL, 100, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	/* Width must be positive (unless in fullscreen mode) */
	assert(pxl_backend_init("test", 0, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);
	assert(pxl_backend_init("test", -1, 100, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	/* Height must be positive (unless in fullscreen mode) */
	assert(pxl_backend_init("test", 100, 0, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);
	assert(pxl_backend_init("test", 100, -1, PXL_BACKEND_HIDDEN) == PXL_E_INVALID_PARAM);

	pxl_backend_deinit();
}

static void
test_pxl_backend_init_fullscreen_zero_size(void) {
	/* In fullscreen mode, width/height can be 0 to use screen resolution */
	assert(pxl_backend_init("test", 0, 0, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Also works with only width = 0 */
	assert(pxl_backend_init("test", 0, 1080, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* And only height = 0 */
	assert(pxl_backend_init("test", 1920, 0, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();
}

static void
test_pxl_backend_deinit_safety(void) {
	/* Deinit without prior init should not crash */
	pxl_backend_deinit();

	/* Double deinit should not crash */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();
	pxl_backend_deinit();
}

/* Flags -------------------------------------------------------------------- */

static void
test_pxl_backend_flags(void) {
	/* Hidden flag should work (avoids display artifacts) */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Fullscreen flag should work (even if not visible due to HIDDEN) */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Centered flag should work */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_CENTERED | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* VSYNC flag should work (may be ignored by some backends) */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_VSYNC | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();

	/* Combined flags should work */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_CENTERED | PXL_BACKEND_VSYNC | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	pxl_backend_deinit();
}

/* Utilities ---------------------------------------------------------------- */

static void
test_pxl_backend_get_window_size_basic(void) {
	int w = 128, h = 64;
	assert(pxl_backend_init("test", w, h, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	int out_w, out_h;
	pxl_backend_get_window_size(&out_w, &out_h);
	assert(out_w == w);
	assert(out_h == h);

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_window_size_fullscreen(void) {
	/* In fullscreen, dimensions should match screen resolution */
	assert(pxl_backend_init("test", 0, 0, PXL_BACKEND_FULLSCREEN | PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	int w, h;
	pxl_backend_get_window_size(&w, &h);
	assert(w > 0);
	assert(h > 0);

	pxl_backend_deinit();
}

static void
test_pxl_backend_large_buffer(void) {
	/* Test that backend handles very large dimensions (beyond typical XShm limits).
	 * This verifies the fallback to heap-allocated buffer works correctly. */
	assert(pxl_backend_init("test", 2000, 2000, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	pxl_buf_t pb;
	assert(pxl_backend_begin_frame(&pb) == PXL_SUCCESS);
	assert(pb.width == 2000);
	assert(pb.height == 2000);
	assert(pb.data != NULL);

	assert(pxl_backend_end_frame() == PXL_SUCCESS);
	pxl_backend_deinit();
}

static void
test_pxl_backend_toggle_fullscreen(void) {
	/* Initialize in windowed mode (with HIDDEN to avoid display artifacts) */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	/* Toggle fullscreen should not crash and should return success */
	assert(pxl_backend_toggle_fullscreen() == PXL_SUCCESS);

	/* Toggle back should also succeed */
	assert(pxl_backend_toggle_fullscreen() == PXL_SUCCESS);

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_time_basic(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	double t1 = pxl_backend_get_time();
	/* Time should be non-negative */
	assert(t1 >= 0.0);

	/* Time should be monotonically increasing */
	double t2 = pxl_backend_get_time();
	assert(t2 >= t1);

	pxl_backend_deinit();
}

static void
test_pxl_backend_poll_events_valid_input(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	pxl_input_t input_state = {0};
	pxl_backend_poll_events(&input_state);

	pxl_backend_deinit();
}

/* Frame flow ---------------------------------------------------------------- */

static void
test_pxl_backend_frame_flow(void) {
	int w = 100, h = 100;
	assert(pxl_backend_init("test", w, h, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	pxl_buf_t pb;
	for (int i = 0; i < 5; i++) {
		assert(pxl_backend_begin_frame(&pb) == PXL_SUCCESS);
		
		/* Validate dimensions on first frame only */
		if (i == 0) {
			assert(pb.width == w);
			assert(pb.height == h);
			assert(pb.stride > 0);
			assert(pb.stride % PXL_BUF_ALIGN == 0);
			assert(pb.data != NULL);
		}

		assert(pxl_backend_end_frame() == PXL_SUCCESS);
	}

	pxl_backend_deinit();
}

/* Text input ---------------------------------------------------------------- */

static void
test_pxl_backend_has_typed_text_empty(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);
	assert(!pxl_backend_has_typed_text());
	pxl_backend_deinit();
}

static void
test_pxl_backend_get_typed_text_empty(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];
	int len = pxl_backend_get_typed_text(buf, sizeof(buf));
	assert(len == 0);
	assert(buf[0] == '\0');

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_typed_text_edge_cases(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];
	/* out_text_max_len = 1 (only room for null terminator) */
	assert(pxl_backend_get_typed_text(buf, 1) == 0);
	assert(buf[0] == '\0');

	pxl_backend_deinit();
}

static void
test_pxl_backend_get_typed_text_null_termination(void) {
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	char buf[32];
	pxl_backend_get_typed_text(buf, sizeof(buf));
	assert(buf[0] == '\0');

	pxl_input_t input = {0};
	pxl_backend_poll_events(&input);

	assert(pxl_backend_get_typed_text(buf, sizeof(buf)) == 0);
	assert(buf[0] == '\0');

	pxl_backend_deinit();
}

/* Example tests -------------------------------------------------------------- */

static void
test_example_pxl_backend_get_typed_text(void) {
	/* This test verifies the example in backend.h compiles and works as documented */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	if (pxl_backend_has_typed_text()) {
		char utf8_buf[32];
		int len = pxl_backend_get_typed_text(utf8_buf, sizeof(utf8_buf));
		assert(len > 0);
		uint32_t rune;
		int consumed = pxl_utf8_decode(utf8_buf, &rune);
		assert(consumed > 0);
	}

	char utf8_buf[32];
	int len = pxl_backend_get_typed_text(utf8_buf, sizeof(utf8_buf));
	assert(len >= 0);
	assert(utf8_buf[0] == '\0' || len > 0);

	pxl_input_t input = {0};
	pxl_backend_poll_events(&input);
	if (pxl_input_state(&input, PXL_KEYB_ENTER)) {
		/* Handle Enter key - verify it compiles */
	}

	pxl_backend_deinit();
}

static void
test_example_pxl_backend_has_typed_text(void) {
	/* This test verifies the example in backend.h for pxl_backend_has_typed_text */
	assert(pxl_backend_init("test", 100, 100, PXL_BACKEND_HIDDEN) == PXL_SUCCESS);

	if (pxl_backend_has_typed_text()) {
		char utf8_buf[32];
		int len = pxl_backend_get_typed_text(utf8_buf, sizeof(utf8_buf));
		assert(len > 0);
	}

	pxl_backend_deinit();
}

/* Main --------------------------------------------------------------------- */

int
main(void) {
	test_pxl_backend_init_invalid_params();
	test_pxl_backend_init_fullscreen_zero_size();
	test_pxl_backend_deinit_safety();
	test_pxl_backend_flags();
	test_pxl_backend_get_window_size_basic();
	test_pxl_backend_get_window_size_fullscreen();
	test_pxl_backend_large_buffer();
	test_pxl_backend_toggle_fullscreen();
	test_pxl_backend_get_time_basic();
	test_pxl_backend_poll_events_valid_input();
	test_pxl_backend_frame_flow();
	test_pxl_backend_has_typed_text_empty();
	test_pxl_backend_get_typed_text_empty();
	test_pxl_backend_get_typed_text_edge_cases();
	test_pxl_backend_get_typed_text_null_termination();
	test_example_pxl_backend_get_typed_text();
	test_example_pxl_backend_has_typed_text();
	return 0;
}
