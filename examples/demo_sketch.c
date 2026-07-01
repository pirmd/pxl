#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "backend.h"
#include "canvas.h"
#include "draw2d.h"
#include "input.h"

#define W 800
#define H 600
#define WHITE 0xFFFFFFFF
#define BLUE  0xFF000080
#define RED   0xFFFF0000
#define PEN_SIZE 10
#define MAX_TRAIL 10000

typedef struct {
    float x, y;
} square_t;

static void
log_fps(double now) {
    static double t0 = 0;
    static int n = 0;
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        float fps = (float)n / (float)(now - t0);
        printf("FPS: %d\r", (int)fps);
        fflush(stdout);
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    if (backend_init("Etch-a-Sketch", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Arrow/Vim keys to draw (Shift for big steps), ESC to clear, Q to quit\n");

    square_t pen = {W/2, H/2};
    square_t pen_prev = pen;
    square_t trail[MAX_TRAIL];
    int trail_len = 0;
    int step_small = 5;
    int step_big = 20;

    input_t in;
    input_init(&in);

    while (!input_is_pressed(&in, IN_KEYB_Q) && !input_pressed(&in.cur, IN_WM_QUIT)) {
        input_next_state(&in);
        backend_poll_events(&in.cur);

        // Clear on Escape
        if (input_was_pressed(&in, IN_KEYB_ESCAPE)) {
            trail_len = 0;
        }

        int step = input_is_pressed(&in, IN_KEYB_LSHIFT) || input_is_pressed(&in, IN_KEYB_RSHIFT) ? step_big : step_small;

        if (input_was_pressed(&in, IN_KEYB_LEFT)  || input_was_pressed(&in, IN_KEYB_H)) pen.x -= step;
        if (input_was_pressed(&in, IN_KEYB_RIGHT) || input_was_pressed(&in, IN_KEYB_L)) pen.x += step;
        if (input_was_pressed(&in, IN_KEYB_UP)    || input_was_pressed(&in, IN_KEYB_K)) pen.y -= step;
        if (input_was_pressed(&in, IN_KEYB_DOWN)  || input_was_pressed(&in, IN_KEYB_J)) pen.y += step;

        // Clamp to screen
        if (pen.x < 0) pen.x = 0;
        if (pen.x + PEN_SIZE > W) pen.x = W - PEN_SIZE;
        if (pen.y < 0) pen.y = 0;
        if (pen.y + PEN_SIZE > H) pen.y = H - PEN_SIZE;

        // Draw line between previous and current pen position
        float dx = pen.x - pen_prev.x;
        float dy = pen.y - pen_prev.y;

        if (dx != 0 || dy != 0) {
            int num_segments = (int)((fabsf(dx) + fabsf(dy)) / PEN_SIZE) + 1;
            float step_x = dx / num_segments;
            float step_y = dy / num_segments;

            for (int i = 1; i <= num_segments; i++) {
                if (trail_len < MAX_TRAIL) {
                    trail[trail_len].x = pen_prev.x + step_x * i;
                    trail[trail_len].y = pen_prev.y + step_y * i;
                    trail_len++;
                }
            }
            pen_prev = pen;
        }

        pixbuf_t pb;
        if (backend_begin_frame(&pb) == PXL_SUCCESS) {
            canvas_t cnv;
            canvas_init(&cnv, &pb);

            canvas_set_color(&cnv, BLUE);
            canvas_clear(&cnv);

            // Draw trail
            canvas_set_color(&cnv, WHITE);
            for (int i = 0; i < trail_len; i++) {
                draw2d_fill_rect(&cnv, (int)trail[i].x, (int)trail[i].y, PEN_SIZE, PEN_SIZE);
            }

            // Current pen position
            canvas_set_color(&cnv, RED);
            draw2d_fill_rect(&cnv, (int)pen.x, (int)pen.y, PEN_SIZE, PEN_SIZE);

            log_fps(backend_get_time());
            backend_end_frame();
        }
    }

    printf("\n");
    backend_deinit();
    return 0;
}
