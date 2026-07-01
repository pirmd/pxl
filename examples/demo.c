#include <stdbool.h>
#include <stdio.h>
#include "backend.h"
#include "canvas.h"
#include "draw2d.h"
#include "stepper.h"
#include "input.h"

#define W 800
#define H 600
#define FPS 60.0f
#define WHITE 0xFFFFFFFF
#define BLUE  0xFF000080

typedef struct {
    float x, y, vx, vy;
    int w, h;
    float speed;
    float prev_x, prev_y;
} Square;

static void handle_input(Square *s, input_state_t *in) {
    s->vx = s->vy = 0;
    if (input_pressed(in, IN_KEYB_LEFT)  || input_pressed(in, IN_KEYB_H)) s->vx = -s->speed;
    if (input_pressed(in, IN_KEYB_RIGHT) || input_pressed(in, IN_KEYB_L)) s->vx =  s->speed;
    if (input_pressed(in, IN_KEYB_UP)    || input_pressed(in, IN_KEYB_K)) s->vy = -s->speed;
    if (input_pressed(in, IN_KEYB_DOWN)  || input_pressed(in, IN_KEYB_J)) s->vy =  s->speed;
}

static void update(Square *s, float dt, int width, int height) {
    s->prev_x = s->x;
    s->prev_y = s->y;
    s->x += s->vx * dt;
    s->y += s->vy * dt;
    if (s->x < 0) s->x = 0;
    if (s->x + s->w > width) s->x = width - s->w;
    if (s->y < 0) s->y = 0;
    if (s->y + s->h > height) s->y = height - s->h;
}

static void render(canvas_t *cnv, const Square *s, float alpha) {
    float x = s->x + (s->prev_x - s->x) * alpha;
    float y = s->y + (s->prev_y - s->y) * alpha;
    canvas_set_color(cnv, BLUE);
    canvas_clear(cnv);
    canvas_set_color(cnv, WHITE);
    draw2d_fill_rect(cnv, (int)x, (int)y, s->w, s->h);
}

static void log_fps(double now) {
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

int main(void) {
    if (backend_init("PXL Demo", W, H, false) != PXL_SUCCESS)
        return 1;

    Square square = {W/2-50, H/2-50, 0, 0, 100, 100, 300.0f, 0, 0};
    time_stepper_t ts;
    stepper_init(&ts, backend_get_time());
    ts.dt = 1.0f / FPS;
    input_state_t in;
    input_init_state(&in);

    while (!input_pressed(&in, IN_KEYB_ESCAPE) && !input_pressed(&in, IN_WM_QUIT)) {
        stepper_sync_time(&ts, backend_get_time());
        backend_poll_events(&in);
        handle_input(&square, &in);

        while (stepper_advance(&ts)) {
            update(&square, (float)ts.dt, W, H);
        }

        pixbuf_t pb;
        if (backend_begin_frame(&pb) == PXL_SUCCESS) {
            canvas_t cnv;
            canvas_init(&cnv, &pb);
            render(&cnv, &square, ts.lerp_factor);
            log_fps(backend_get_time());
            backend_end_frame();
        }
    }

    backend_deinit();
    return 0;
}
