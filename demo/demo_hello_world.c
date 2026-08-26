#include "pxl.h"

int
main(void) {
	pxl_app_t app = {
		.title = "Hello World - PXL",
		.width = 800,
		.height = 600,
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	while (pxl_app_advance_wait(&app)) {
		if (pxl_app_was_pressed(&app, PXL_KEYB_ESCAPE))
			break;

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			/* Clear to dark gray */
			pxl_canvas_set_color(&cnv, 0xFF202020);
			pxl_canvas_clear(&cnv);

			/* Draw a colored rectangle */
			pxl_canvas_set_color(&cnv, 0xFF0080FF);
			pxl_fill_rect(&cnv, 200, 200, 400, 200);

			/* Draw hello text */
			pxl_canvas_set_color(&cnv, 0xFFFFFFFF);
			pxl_draw_str(&cnv, 300, 280, "Hello, PXL!");

			(void)pxl_backend_end_frame();
		}
	}

	pxl_app_deinit(&app);
	return 0;
}
