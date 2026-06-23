#include <stdbool.h>
#include <stdio.h>

#include "backend.h"
#include "canvas.h"
#include "color.h"
#include "draw2d.h"
#include "err.h"
#include "input.h"
#include "pixbuf.h"

#define WHITE color_argb(255, 255, 255, 255)
#define BLUE  color_argb(255,   0,   0, 128)
#define GREEN color_argb(255,   0, 128,   0)

typedef struct {
    float p_x, p_y;
	int   p_w, p_h; 

    float p_v_x, p_v_y;

	float speed;
	int width, height;
} world_t;

world_t
update_world(const world_t *cur, float dt) {
	world_t nxt = *cur;

	nxt.p_x = cur->p_x + cur->p_v_x * dt;

	if (nxt.p_x < 0) nxt.p_x = 0; 
	if (nxt.p_x + nxt.p_w > nxt.width) nxt.p_x = nxt.width - nxt.p_w - 1; 

	nxt.p_y = cur->p_y + cur->p_v_y * dt;
	if (nxt.p_y < 0) nxt.p_y = 0; 
	if (nxt.p_y + nxt.p_h > nxt.height) nxt.p_y = nxt.height - nxt.p_h - 1; 

	nxt.p_v_x = 0.0;
	nxt.p_v_y = 0.0;

	return nxt;
}

world_t
interpolate_world(const world_t *cur, const world_t *prev, float alpha) {
	world_t w = *cur;

    w.p_x = cur->p_x + (cur->p_x - prev->p_x) * alpha;
    w.p_y = cur->p_y + (cur->p_y - prev->p_y) * alpha;

	return w;
}

void
render_world(canvas_t *cnv, const world_t *world) {
	canvas_set_color(cnv, BLUE);
	canvas_clear(cnv);
	draw2d_circle(cnv, 100, 100, 50);

	canvas_set_color(cnv, WHITE);
	draw2d_fill_rect(cnv, world->p_x, world->p_y, world->p_w, world->p_h);
}

void
process_input(input_t *input, world_t *cur, bool *running) {
	if (input_is_down(input, KEY_ESCAPE)) {
		*running = false;
	}

	if (input_is_down(input, KEY_LEFT))  {
		cur->p_v_x = -cur->speed;
	}

	if (input_is_down(input, KEY_RIGHT)) {
		cur->p_v_x = cur->speed;
	}

	if (input_is_down(input, KEY_UP))    {
		cur->p_v_y = -cur->speed;
	}

	if (input_is_down(input, KEY_DOWN))  {
		cur->p_v_y = cur->speed;
	}
}


static struct {
    double last_time;      /* temps du dernier calcul de FPS   */
    int    frame_cnt;      /* nombre d'images depuis last_time */
    int    fps;            /* FPS calculé                     */
} g_fps = {0};

static void
update_fps(void) {
    double now = backend_get_time();

    if (g_fps.last_time == 0.0) {
        g_fps.last_time = now;
        return;
    }

    ++g_fps.frame_cnt;

    if (now - g_fps.last_time >= 1.0) {
        g_fps.fps = g_fps.frame_cnt;
        g_fps.frame_cnt = 0;
        g_fps.last_time = now;
    }
}

static inline int
get_fps(void) {
   	return g_fps.fps;
}

int
main(void) {
	const int W = 800, H = 600;

	if (backend_init("PXL Demo", W, H, false) != PXL_SUCCESS) {
		return EXIT_FAILURE;
	}
	input_t input = {0};

	world_t world_prev = {0};
	world_t world_cur = {
		.p_w  = 100,
		.p_h  = 100,
		.speed = 300,
		.width  = W,
		.height = H
	};

    bool running = true;
    while (running) {
		backend_new_frame();

		if (backend_poll_events(&input)) {
			process_input(&input, &world_cur, &running);
		}

		float dt;
        while (backend_next_physics_step(&dt)) {
			world_prev = world_cur;
            world_cur = update_world(&world_cur, dt);
        }

        pixbuf_t pb;
        if (backend_begin_frame(&pb) == PXL_SUCCESS) {
            canvas_t cnv;
            canvas_init(&cnv, &pb);

			world_t w = interpolate_world(&world_cur, &world_prev, backend_get_alpha());
            render_world(&cnv, &w);

			if (input.mouse_left) {
				canvas_set_color(&cnv, GREEN);
				draw2d_line(&cnv, 0, 0, input.mouse_x, input.mouse_y);
			}

			update_fps();
			printf("FPS: %5d\r", get_fps());
			fflush(stdout);

            backend_end_frame();
        }

    }

	backend_deinit();
	return 0;
}

