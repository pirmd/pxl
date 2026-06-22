#include <stdbool.h>

#include "backend.h"
#include "canvas.h"
#include "color.h"
#include "draw2d.h"
#include "err.h"
#include "input.h"
#include "pixbuf.h"

int main(void) {
	if (backend_init("PXL Demo", 800, 600, false) != PXL_SUCCESS) {
		return 1;
	}

	input_t input = {0};

	pix_t white_color = color_argb(255, 255, 255, 255);   // White
	pix_t  blue_color = color_argb(255,   0,   0, 128);   // Blue
	pix_t green_color = color_argb(255,   0, 128,   0);   // Green
    
	int x = 0, y = 0, speed = 5;

    bool running = true;
    while (running) {
		if (backend_poll_events(&input)) {
			if (input_is_down(&input, KEY_LEFT))  x -= speed;
			if (input_is_down(&input, KEY_RIGHT)) x += speed;
			if (input_is_down(&input, KEY_UP))    y -= speed;
			if (input_is_down(&input, KEY_DOWN))  y += speed;

			if (input_is_down(&input, KEY_ESCAPE)) {
			   	running = false;
				continue;
			}
		}

        pixbuf_t pb;
        if (backend_begin_frame(&pb) == PXL_SUCCESS) {
            canvas_t cnv;
            canvas_init(&cnv, &pb);

			if (x < 0) x = 0;
			if (x + 100 >= pb.width) x = pb.width - 101;
			if (y < 0) y = 0;
			if (y + 100 >= pb.height) y = pb.height - 101;

            canvas_set_color(&cnv, blue_color);
            canvas_clear(&cnv);
            draw2d_circle(&cnv, 100, 100, 50);

			canvas_set_color(&cnv, white_color);
			draw2d_fill_rect(&cnv, x, y, 100, 100);

			if (input.mouse_left) {
				canvas_set_color(&cnv, green_color);
				draw2d_line(&cnv, 0, 0, input.mouse_x, input.mouse_y);
			}

            backend_end_frame();
        }
    }

	backend_deinit();
	return 0;
}

