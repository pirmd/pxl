/*
 * PXL Minimal Example: Hello World
 *
 * Simplest possible PXL program:
 *   - Creates a window
 *   - Draws a colored rectangle and text
 *   - Exits on ESC
 *
 * Note: Uses pxl_app_advance_wait() for simplicity (no physics loop).
 * For games with movement/physics, see demo_pong.c.
 */

#include "pxl.h"

int
main(void) {
	pxl_app_t app = {
		.title = "Hello World - PXL",
		.width = 800,
		.height = 600
		/* physics_dt not needed for static drawing (defaults to 0) */
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	/* Main loop: advance_wait blocks until next frame (simpler than stepper) */
	while (pxl_app_advance_wait(&app)) {
		if (pxl_app_was_pressed(&app, PXL_KEYB_ESCAPE))
			break;

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			/* Clear screen */
			pxl_canvas_set_color(&cnv, 0xFF202020); /* Dark gray */
			pxl_canvas_clear(&cnv);

			/* Draw a blue rectangle */
			pxl_canvas_set_color(&cnv, 0xFF0080FF);
			pxl_fill_rect(&cnv, 200, 200, 400, 200);

			/* Draw text (using built-in ASCII font from text_basic.h) */
			pxl_canvas_set_color(&cnv, 0xFFFFFFFF); /* White */
			pxl_draw_str(&cnv, 300, 280, "Hello, PXL!");

			(void)pxl_backend_end_frame();
		}
	}

	pxl_app_deinit(&app);
	return 0;
}
