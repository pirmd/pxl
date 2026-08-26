#include "pxl.h"

int
main(void) {
	pxl_input_t in = {0};
	pxl_backend_init("Hello World - PXL", 800, 600, 0);

	while (pxl_input_state(&in, PXL_KEYB_ESCAPE) == 0 &&
	       pxl_input_state(&in, PXL_WM_QUIT) == 0) {
		pxl_backend_poll_events(&in);

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

	pxl_backend_deinit();
	return 0;
}
