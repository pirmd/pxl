#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pxl.h"

#define W 800
#define H 600
#define FPS 60.0f
#define WHITE 0xFFFFFFFF
#define BLUE  0xFF000080
#define RED   0xFF0000FF

/* Pong game state */
typedef struct {
    /* Paddles */
    struct {
        float x, y;
        int w, h;
        float speed;
        float vy;
    } paddle_left, paddle_right;

    /* Ball */
    struct {
        float x, y;
        int radius;
        float vx, vy;
        float speed;
    } ball;

    /* Scores */
    int score_left;
    int score_right;
} pong_t;

static void
handle_input(pong_t *p, pxl_input_t *in) {
    p->paddle_left.vy = 0;
    p->paddle_right.vy = 0;  /* Will be set by AI */

    /* Left paddle: vim keys (k=up, j=down) or arrows (up/down) */
    if (pxl_input_is_pressed(in, PXL_KEYB_K) || pxl_input_is_pressed(in, PXL_KEYB_UP)) {
        p->paddle_left.vy = -p->paddle_left.speed;
    }
    if (pxl_input_is_pressed(in, PXL_KEYB_J) || pxl_input_is_pressed(in, PXL_KEYB_DOWN)) {
        p->paddle_left.vy = p->paddle_left.speed;
    }
}

static void
reset_ball(pong_t *p) {
    p->ball.x = W / 2.0f;
    p->ball.y = H / 2.0f;
    p->ball.vx = (arc4random() % 2 == 0 ? 1.0f : -1.0f) * p->ball.speed;
    p->ball.vy = ((float)(arc4random() % 100) / 100.0f - 0.5f) * p->ball.speed * 1.5f;
}

static void
pong_interpolate(pong_t *out, const pong_t *prev, const pong_t *cur, float alpha) {
    out->paddle_left.x = cur->paddle_left.x;
    out->paddle_left.y = prev->paddle_left.y + (cur->paddle_left.y - prev->paddle_left.y) * alpha;
    out->paddle_left.w = cur->paddle_left.w;
    out->paddle_left.h = cur->paddle_left.h;

    out->paddle_right.x = cur->paddle_right.x;
    out->paddle_right.y = prev->paddle_right.y + (cur->paddle_right.y - prev->paddle_right.y) * alpha;
    out->paddle_right.w = cur->paddle_right.w;
    out->paddle_right.h = cur->paddle_right.h;

    out->ball.x = prev->ball.x + (cur->ball.x - prev->ball.x) * alpha;
    out->ball.y = prev->ball.y + (cur->ball.y - prev->ball.y) * alpha;
    out->ball.radius = cur->ball.radius;
    out->ball.speed = cur->ball.speed;

    out->score_left = cur->score_left;
    out->score_right = cur->score_right;
}

static void
ai_decide(pong_t *p) {
    /* Simple AI: predict ball position on right side */
    float target_y;
    
    /* Ball going right: direct prediction */
    if (p->ball.vx > 0) {
        float time_to_right = (p->paddle_right.x - p->ball.x) / p->ball.vx;
        target_y = p->ball.y + p->ball.vy * time_to_right;
    }
    /* Ball going left: move to center (simplest reliable strategy) */
    else {
        target_y = H / 2.0f;
    }
    
    /* Clamp to playable area */
    target_y = fmaxf(p->paddle_right.h / 2.0f, 
                   fminf(H - p->paddle_right.h / 2.0f, target_y));
    
    /* Calculate required movement */
    float error = target_y - (p->paddle_right.y + p->paddle_right.h / 2.0f);
    
    if (fabsf(error) > 2.0f) {
        p->paddle_right.vy = (error < 0 ? -1.0f : 1.0f) * p->paddle_right.speed;
    } else {
        p->paddle_right.vy = 0;
    }
}

static void
update_pong(pong_t *p, float dt) {
    /* AI decision (separate from physics update) */
    ai_decide(p);

    /* Update paddles */
    p->paddle_left.y += p->paddle_left.vy * dt;
    p->paddle_right.y += p->paddle_right.vy * dt;

    /* Clamp paddles to screen */
    if (p->paddle_left.y < 0) p->paddle_left.y = 0;
    if (p->paddle_left.y + p->paddle_left.h > H) p->paddle_left.y = H - p->paddle_left.h;
    if (p->paddle_right.y < 0) p->paddle_right.y = 0;
    if (p->paddle_right.y + p->paddle_right.h > H) p->paddle_right.y = H - p->paddle_right.h;

    /* Update ball */
    p->ball.x += p->ball.vx * dt;
    p->ball.y += p->ball.vy * dt;

    /* Ball collision with top and bottom */
    if (p->ball.y - p->ball.radius < 0) {
        p->ball.y = p->ball.radius;
        p->ball.vy = -p->ball.vy;
    }
    if (p->ball.y + p->ball.radius > H) {
        p->ball.y = H - p->ball.radius;
        p->ball.vy = -p->ball.vy;
    }

    /* Ball collision with paddles */
    /* Left paddle */
    if (p->ball.x - p->ball.radius < p->paddle_left.x + p->paddle_left.w &&
        p->ball.y + p->ball.radius > p->paddle_left.y &&
        p->ball.y - p->ball.radius < p->paddle_left.y + p->paddle_left.h) {
        p->ball.x = p->paddle_left.x + p->paddle_left.w + p->ball.radius;
        p->ball.vx = -p->ball.vx * 1.05f;
        float paddle_center = p->paddle_left.y + p->paddle_left.h / 2.0f;
        float hit_pos = (p->ball.y - paddle_center) / (p->paddle_left.h / 2.0f);
        p->ball.vy = hit_pos * p->ball.speed * 0.8f;
    }

    /* Right paddle */
    if (p->ball.x + p->ball.radius > p->paddle_right.x &&
        p->ball.y + p->ball.radius > p->paddle_right.y &&
        p->ball.y - p->ball.radius < p->paddle_right.y + p->paddle_right.h) {
        p->ball.x = p->paddle_right.x - p->ball.radius;
        p->ball.vx = -p->ball.vx * 1.05f;
        float paddle_center = p->paddle_right.y + p->paddle_right.h / 2.0f;
        float hit_pos = (p->ball.y - paddle_center) / (p->paddle_right.h / 2.0f);
        p->ball.vy = hit_pos * p->ball.speed * 0.8f;
    }

    /* Ball out of bounds (score) */
    if (p->ball.x - p->ball.radius < 0) {
        p->score_right++;
        reset_ball(p);
    }
    if (p->ball.x + p->ball.radius > W) {
        p->score_left++;
        reset_ball(p);
    }
}

static void
draw_score(pxl_canvas_t *cnv, const pong_t *p) {
    /* Draw dash separator */
    pxl_canvas_set_color(cnv, WHITE);
    pxl_fill_rect(cnv, W/2 - 10, 20, 20, 4);
    
    /* Simple score indicators using rectangles */
    /* Left score: draw one rect per point */
    pxl_canvas_set_color(cnv, WHITE);
    for (int i = 0; i < p->score_left; i++) {
        pxl_fill_rect(cnv, W/2 - 40 - i * 15, 10, 8, 20);
    }
    
    /* Right score: draw one rect per point */
    for (int i = 0; i < p->score_right; i++) {
        pxl_fill_rect(cnv, W/2 + 25 + i * 15, 10, 8, 20);
    }
}

static void
render_pong(pxl_canvas_t *cnv, const pong_t *p) {
    /* Clear */
    pxl_canvas_set_color(cnv, BLUE);
    pxl_canvas_clear(cnv);

    /* Draw score */
    draw_score(cnv, p);

    /* Draw center line */
    pxl_canvas_set_color(cnv, WHITE);
    for (int y = 0; y < H; y += 30) {
        pxl_fill_rect(cnv, W/2 - 2, y, 4, 20);
    }

    /* Draw paddles */
    pxl_canvas_set_color(cnv, WHITE);
    pxl_fill_rect(cnv, (int)p->paddle_left.x, (int)p->paddle_left.y, 
                     p->paddle_left.w, p->paddle_left.h);
    pxl_fill_rect(cnv, (int)p->paddle_right.x, (int)p->paddle_right.y, 
                     p->paddle_right.w, p->paddle_right.h);

    /* Draw ball */
    pxl_fill_circle(cnv, (int)p->ball.x, (int)p->ball.y, p->ball.radius);
}

static void
render_debug_hud(pxl_canvas_t *cnv, int fps, pxl_input_state_t *in) {
	pxl_buf_t *pb = cnv->pb;
	int m_x = in->mouse_x, m_y = in->mouse_y;

    char hud_str[64];
	
	if (m_x >= 0 && m_x < pb->width && m_y >= 0 && m_y < pb->height) {
		pxl_t color = *pxl_buf_ptr(pb, m_x, m_y);
		snprintf(hud_str, sizeof(hud_str), "FPS: %d | Mouse: %d,%d | Pixel: #%06X",
				fps, m_x, m_y, color & 0x00FFFFFF);
	} else {
		snprintf(hud_str, sizeof(hud_str), "FPS: %d | Mouse: n/a | Pixel: n/a", fps);
	}

    pxl_canvas_set_color(cnv, WHITE);
    pxl_draw_str(cnv, 10, H - 15, hud_str);
}

static void
update_fps(double now, int *current_fps) {
    static double t0 = 0;
    static int n = 0;
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        *current_fps = (int)((float)n / (float)(now - t0));
        n = 0;
        t0 = now;
    }
}

static void
init_pong(pong_t *p) {
    /* Paddles */
    p->paddle_left.x = 20;
    p->paddle_left.y = H/2 - 50;
    p->paddle_left.w = 15;
    p->paddle_left.h = 100;
    p->paddle_left.speed = 400.0f;

    p->paddle_right.x = W - 20 - 15;
    p->paddle_right.y = H/2 - 50;
    p->paddle_right.w = 15;
    p->paddle_right.h = 100;
    p->paddle_right.speed = 400.0f;

    /* Ball */
    p->ball.radius = 8;
    p->ball.speed = 300.0f;
    reset_ball(p);

    /* Scores */
    p->score_left = 0;
    p->score_right = 0;
}

int
main(void) {
    if (pxl_backend_init("PXL Pong", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Pong game. Vim keys: J=down, K=up. CTRL=show debug HUD, ESC=quit\n");

    pong_t pong, pong_prev;
    init_pong(&pong);
    pong_prev = pong;

    pxl_time_stepper_t ts;
    ts.dt = 1.0f / FPS;
    pxl_stepper_init(&ts, pxl_backend_get_time());

    pxl_input_t in;
    pxl_input_init(&in);

    int current_fps = 0;

    while (!pxl_input_is_pressed(&in, PXL_KEYB_ESCAPE) && !pxl_input_is_pressed(&in, PXL_WM_QUIT)) {
        pxl_stepper_sync_time(&ts, pxl_backend_get_time());
        pxl_input_next_state(&in);
        pxl_backend_poll_events(&in.cur);
        handle_input(&pong, &in);

        while (pxl_stepper_advance(&ts)) {
            pong_prev = pong;
            update_pong(&pong, (float)ts.dt);
        }

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            pong_t pong_interpolated;
            pong_interpolate(&pong_interpolated, &pong_prev, &pong, ts.lerp_factor);
            render_pong(&cnv, &pong_interpolated);
            
			/* Show HUD when CTRL is pressed */
			if (pxl_input_is_pressed(&in, PXL_KEYB_LCTRL) || pxl_input_is_pressed(&in, PXL_KEYB_RCTRL)) {
				render_debug_hud(&cnv, current_fps, &in.cur);
			}

            pxl_backend_end_frame();
        }

		update_fps(pxl_backend_get_time(), &current_fps);
    }

    printf("Final Score: %d - %d\n", pong.score_left, pong.score_right);
    pxl_backend_deinit();
    return 0;
}
