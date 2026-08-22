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

static const uint8_t pxl_font_big_digit[10][20][2] = {
	{ /* 0 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 1 */
		{0xF0, 0x03},
		{0xF0, 0x03},
		{0xF0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xC0, 0x03},
		{0xF0, 0x0F},
		{0xF0, 0x0F},
		{0xF0, 0x0F},
	},

	{ /* 2 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 3 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 4 */
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
	},

	{ /* 5 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 6 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0x07, 0x00},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 7 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
	},

	{ /* 8 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},

	{ /* 9 */
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0x07, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0x00, 0xE0},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
		{0xFF, 0xFF},
	},
};

const pxl_font_t pxl_font_pong = {
	.bitmask = {
		.data   = (const uint8_t *)pxl_font_big_digit,
		.width  = 16,
		.height = 200,    /* 10 chars * 20 rows each */
		.stride = 2 
	},

	.rune_start = 48,     /* '0' */
	.rune_end  = 57,      /* '9' */
	.fallback_rune = 0,   /* Skip out-of-range chars */
	.tracking = 2,
	.leading = 22,
	.glyph_height = 20,   /* Each char is 20 pixels tall */
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL,
};

typedef struct {
    struct {
        float x, y;
        int w, h;
        float speed;
        float vy;
    } paddle_left, paddle_right;

    struct {
        float x, y;
        int radius;
        float vx, vy;
        float speed;
    } ball;

    int score_left;
    int score_right;
} pong_t;

static void
reset_ball(pong_t *p) {
    p->ball.x = W / 2.0f;
    p->ball.y = H / 2.0f;
    p->ball.vx = (arc4random() % 2 == 0 ? 1.0f : -1.0f) * p->ball.speed;
    p->ball.vy = ((float)(arc4random() % 100) / 100.0f - 0.5f) * p->ball.speed * 1.5f;
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

static void
update_pong(pong_t *p, float dt) {
    /* Update paddles */
    p->paddle_left.y  += p->paddle_left.vy * dt;
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
handle_input(pong_t *p, pxl_input_t *in) {
    p->paddle_left.vy = 0;
    p->paddle_right.vy = 0;  /* Will be set by AI */

    /* Left paddle: vim keys (k=up, j=down) or arrows (up/down) */
    if (pxl_input_state(in, PXL_KEYB_K) == 1 || pxl_input_state(in, PXL_KEYB_UP) == 1) {
        p->paddle_left.vy = -p->paddle_left.speed;
    }
    if (pxl_input_state(in, PXL_KEYB_J) == 1 || pxl_input_state(in, PXL_KEYB_DOWN) == 1) {
        p->paddle_left.vy = p->paddle_left.speed;
    }
}

static void
interpolate_pong(pong_t *out, const pong_t *prev, const pong_t *cur, float alpha) {
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
draw_score(pxl_canvas_t *cnv, const pong_t *p) {
    pxl_writer_t w;
    const pxl_font_t *fonts[] = {&pxl_font_pong};
    pxl_writer_init(&w, fonts, 1);

	/* Use text_bounds to calculate width for a typical score (e.g., "999") */
	pxl_rect_t max_score_bounds = pxl_text_bounds(&w, "999");
	const int score_half_width = max_score_bounds.w / 2;
    const int score_y     = 30;

    pxl_canvas_set_color(cnv, WHITE);
    
    char score_str[8];
    snprintf(score_str, sizeof(score_str), "%d", p->score_left);
    pxl_writer_set_cursor(&w, W/4 - score_half_width, score_y);
    pxl_draw_text(cnv, &w, score_str);

    snprintf(score_str, sizeof(score_str), "%d", p->score_right);
    pxl_writer_set_cursor(&w, 3 * W / 4 - score_half_width, score_y);
    pxl_draw_text(cnv, &w, score_str);
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

/*
 * AI
 */

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


/*
 * Application
 */

struct {
	pxl_input_t in_prev;
	pxl_input_t in_curr;

	pxl_time_stepper_t stepper;
	bool paused;

	pong_t pong;
	pong_t pong_prev;
	int current_fps;
} app;

static inline bool
is_pressed(pxl_input_code_t code) {
	return pxl_input_state(&app.in_curr, code) == 1;
}

static inline bool
was_pressed(pxl_input_code_t code) {
	return (pxl_input_state(&app.in_prev, code) == 0) && (pxl_input_state(&app.in_curr, code) == 1);
}

static inline bool
is_paused(void) {
	bool auto_paused = pxl_input_state(&app.in_curr, PXL_WM_FOCUS_LOST) || pxl_input_state(&app.in_curr, PXL_WM_MOUSE_FOCUS_LOST);
	app.stepper.paused = app.paused || auto_paused;
	return app.paused || auto_paused;
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
render_debug_hud(pxl_canvas_t *cnv, int fps, pxl_input_t *in) {
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

int
main(void) {
    if (pxl_backend_init("PXL Pong", W, H, 0) != PXL_SUCCESS)
        return 1;

    printf("Pong game. Vim keys: J=down, K=up. CTRL=HUD, P=pause, ESC=quit\n");

    app.stepper.dt = 1.0f / FPS;
    pxl_stepper_init(&app.stepper, pxl_backend_get_time());

    init_pong(&app.pong);
    app.pong_prev = app.pong;
    app.current_fps = 0;

    while (!is_pressed(PXL_KEYB_ESCAPE) && !is_pressed(PXL_WM_QUIT)) {
        app.in_prev = app.in_curr;
        pxl_backend_poll_events(&app.in_curr);

        if (was_pressed(PXL_KEYB_P)) {
            app.paused = !app.paused;
        }

        pxl_stepper_sync_time(&app.stepper, pxl_backend_get_time());
        handle_input(&app.pong, &app.in_curr);

        if (!is_paused()) {
            while (pxl_stepper_advance(&app.stepper)) {
				ai_decide(&app.pong);

                app.pong_prev = app.pong;
                update_pong(&app.pong, (float)app.stepper.dt);
            }
        }

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            pong_t pong_interpolated;
            interpolate_pong(&pong_interpolated, &app.pong_prev, &app.pong, app.stepper.lerp_factor);
            render_pong(&cnv, &pong_interpolated);
            
			/* Show HUD when CTRL is pressed */
			if (is_pressed(PXL_KEYB_LCTRL) || is_pressed(PXL_KEYB_RCTRL)) {
				render_debug_hud(&cnv, app.current_fps, &app.in_curr);
			}

			/* Pause indicator */
			if (is_paused()) {
				printf("PAUSED | Press P to resume\r");
				fflush(stdout);
			}

            pxl_backend_end_frame();
        }

		update_fps(pxl_backend_get_time(), &app.current_fps);
    }

    printf("\nFinal Score: %d - %d\n", app.pong.score_left, app.pong.score_right);
    pxl_backend_deinit();
    return 0;
}
