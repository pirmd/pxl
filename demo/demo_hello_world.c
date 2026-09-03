/*
 * PXL Minimal Example: Hello World with Text Alignment
 *
 * Demonstrates text alignment helpers:
 *   - pxl_align_x/pxl_align_y for positioning
 *   - pxl_text_bounds_n for multi-line text
 *   - pxl_draw_text_n for truncated text
 *
 * Note: Uses pxl_app_advance_wait() for simplicity (no physics loop).
 * For games with movement/physics, see demo_pong.c.
 */

#include "pxl.h"
#include "text.h"

int
main(void) {
	pxl_app_t app = {
		.title = "Hello World with Text Alignment - PXL",
		.width = 800,
		.height = 600
		/* physics_dt not needed for static drawing (defaults to 0) */
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	/* Create a writer for text alignment */
	pxl_writer_t writer;
	const pxl_font_t *fonts[] = {&pxl_font_text_basic};
	pxl_writer_init(&writer, fonts, 1);

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

			/* Draw a blue rectangle as container */
			pxl_canvas_set_color(&cnv, 0xFF0080FF);
			pxl_fill_rect(&cnv, 100, 100, 600, 400);

			/* Draw centered text using alignment helpers */
			pxl_canvas_set_color(&cnv, 0xFFFFFFFF); /* White */
			const char *title = "Text Alignment Demo";
			pxl_rect_t title_bounds = pxl_text_bounds(&writer, title);
			int title_x = pxl_align_x(100, 600, title_bounds.w, PXL_ALIGN_CENTER);
			pxl_writer_set_cursor(&writer, title_x, 150);
			pxl_draw_text(&cnv, &writer, title);

			/* Draw right-aligned text */
			const char *subtitle = "Right Aligned Text";
			pxl_rect_t sub_bounds = pxl_text_bounds(&writer, subtitle);
			int sub_x = pxl_align_x(100, 600, sub_bounds.w, PXL_ALIGN_RIGHT);
			pxl_writer_set_cursor(&writer, sub_x, 200);
			pxl_draw_text(&cnv, &writer, subtitle);

			/* Draw multi-line text with per-line alignment */
			const char *multiline = "Line 1\nLonger Line 2\nShort";
			const char *line_start = multiline;
			int line_y = 280;

			while (*line_start) {
				const char *line_end = line_start;
				while (*line_end && *line_end != '\n') {
					line_end++;
				}

				size_t line_bytes = line_end - line_start;
				pxl_rect_t line_bounds = pxl_text_bounds_n(&writer, line_start, line_bytes);
				
				/* Center each line individually */
				int line_x = pxl_align_x(100, 600, line_bounds.w, PXL_ALIGN_CENTER);
				pxl_writer_set_cursor(&writer, line_x, line_y);
				pxl_draw_text_n(&cnv, &writer, line_start, line_bytes);

				line_y += pxl_font_text_basic.glyph_height + pxl_font_text_basic.leading;
				line_start = (*line_end == '\n') ? line_end + 1 : line_end;
			}

			/* Draw left-aligned text */
			const char *footer = "Left Aligned Footer";
			pxl_rect_t footer_bounds = pxl_text_bounds(&writer, footer);
			int footer_x = pxl_align_x(100, 600, footer_bounds.w, PXL_ALIGN_LEFT);
			pxl_writer_set_cursor(&writer, footer_x, 450);
			pxl_draw_text(&cnv, &writer, footer);

			(void)pxl_backend_end_frame();
		}
	}

	pxl_app_deinit(&app);
	return 0;
}
