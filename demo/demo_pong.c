#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "pxl.h"
#include "font_9x15.h"

/* Simple portable PRNG - LCG */
static uint32_t rng_state = 0;

static uint32_t rng(void) {
	if (rng_state == 0) rng_state = (uint32_t)time(NULL);
	rng_state = rng_state * 1664525u + 1013904223u;
	return rng_state;
}

#define W 800
#define H 600
#define FPS 60.0f

/* Font */
static const pxl_font_t *pong_fonts[] = { &font_9x15_latin };

/* Scaling factors */
#define SCORE_ZOOM  3
#define PAUSE_ZOOM  4

/* Palette */
typedef struct {
    pxl_t fg;
    pxl_t bg;
} palette_t;

static palette_t palette_normal = { .fg = 0xFFFFFFFF, .bg = 0xFF000080 };
static palette_t palette_paused;

/* Convert color to grayscale (perceptual luminance: 0.299R + 0.587G + 0.114B) */
static pxl_t
color_grayscale(pxl_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t y = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    return (a << 24) | (y << 16) | (y << 8) | y;
}

static void
pxl_draw_bitmask_scaled(pxl_canvas_t *cnv, int scale, const pxl_bitmask_t *bm,
	pxl_rect_t bm_r, int cnv_x, int cnv_y) {
	uint32_t color = cnv->color;
	for (int gy = 0; gy < bm_r.h; gy++) {
		for (int gx = 0; gx < bm_r.w; gx++) {
			int byte_idx = (bm_r.y + gy) * bm->stride + (bm_r.x + gx) / 8;
			uint8_t byte = bm->data[byte_idx];
			int bit = (bm_r.x + gx) % 8;
			if (byte & (1 << bit)) {
				for (int sy = 0; sy < scale; sy++) {
					for (int sx = 0; sx < scale; sx++) {
						*pxl_buf_ptr(cnv->pb, cnv_x + gx * scale + sx, cnv_y + gy * scale + sy) = color;
					}
				}
			}
		}
	}
}

/* Measure the scaled pixel width of an ASCII string using a bitmask font.
 * Used to center text (score, pause banner) before drawing it.
 */
static int
measure_text_scaled(const pxl_font_t *font, const char *str, int scale) {
	int total_w = 0;
	for (int i = 0; str[i]; i++) {
		uint32_t rune = (uint8_t)str[i];
		int rune_idx = (int)(rune - font->rune_start);
		if (rune_idx >= 0 && rune_idx <= (int)(font->rune_end - font->rune_start)) {
			int gw = font->glyph_widths ? font->glyph_widths[rune_idx] : font->bitmask.width;
			total_w += gw * scale;
		}
	}
	return total_w;
}

/* Draw an ASCII string with a bitmask font at (x, y), scaled by `scale`.
 * Factored out of render_pause()/draw_score() which both drew scaled text
 * with the same glyph-lookup loop.
 */
static void
draw_text_scaled(pxl_canvas_t *cnv, const pxl_font_t *font, const char *str, int scale, int x, int y) {
	int glyph_w = font->bitmask.width;
	int glyph_h = font->glyph_height;
	pxl_rect_t bm_r = { .x = 0, .y = 0, .w = glyph_w, .h = glyph_h };
	int cur_x = x;
	for (int i = 0; str[i]; i++) {
		uint32_t rune = (uint8_t)str[i];
		int rune_idx = (int)(rune - font->rune_start);
		if (rune_idx < 0 || rune_idx > (int)(font->rune_end - font->rune_start)) continue;
		bm_r.y = rune_idx * glyph_h;
		int gw = font->glyph_widths ? font->glyph_widths[rune_idx] : glyph_w;
		bm_r.w = gw;
		pxl_draw_bitmask_scaled(cnv, scale, &font->bitmask, bm_r, cur_x, y);
		cur_x += gw * scale;
	}
}

static struct {
	pxl_input_t in_prev;
	pxl_input_t in_curr;

	pxl_time_stepper_t stepper;
	bool paused;

	float speed_factor;  /* 1.0 = normal, 2.0 = 2x, 0.5 = 0.5x */
	int current_fps;
	palette_t *palette;  /* Pointer to active palette */
} app;

typedef enum {
    AI_BEGINNER,
    AI_EASY,
    AI_MEDIUM,
    AI_HARD,
    AI_IMPOSSIBLE
} ai_difficulty_t;

static const char *ai_difficulty_names[] = {"BEGINNER", "EASY", "MEDIUM", "HARD", "IMPOSSIBLE"};

typedef struct {
    float reaction_threshold;
    float prediction_bias;
    bool use_persistent_error;
    float error_range;
    int error_duration_min;
    int error_duration_max;
} ai_config_t;

typedef struct {
    ai_config_t config;
    float current_error;
    int error_timer;
} ai_t;

static ai_t ai = {0};

/* Difficulty presets */
static const ai_config_t ai_configs[] = {
	/* BEGINNER */ {
		.reaction_threshold = 20.0f,
		.prediction_bias = 0.0f,
		.use_persistent_error = true,
		.error_range = 25.0f,
		.error_duration_min = 60,
		.error_duration_max = 120
	},
	/* EASY */ {
		.reaction_threshold = 10.0f,
		.prediction_bias = -20.0f,
		.use_persistent_error = false,
		.error_range = 0.0f,
		.error_duration_min = 0,
		.error_duration_max = 0
	},
	/* MEDIUM */ {
		.reaction_threshold = 3.0f,
		.prediction_bias = 0.0f,
		.use_persistent_error = true,
		.error_range = 10.0f,
		.error_duration_min = 30,
		.error_duration_max = 60
	},
	/* HARD */ {
		.reaction_threshold = 1.0f,
		.prediction_bias = 0.0f,
		.use_persistent_error = false,
		.error_range = 0.0f,
		.error_duration_min = 0,
		.error_duration_max = 0
	},
	/* IMPOSSIBLE */ {
		.reaction_threshold = 0.0f,
		.prediction_bias = 0.0f,
		.use_persistent_error = false,
		.error_range = 0.0f,
		.error_duration_min = 0,
		.error_duration_max = 0
	}
};

static ai_difficulty_t ai_difficulty = AI_MEDIUM;
static bool two_players_mode = false;

static void ai_set_difficulty(ai_difficulty_t difficulty);

static inline bool
is_pressed(pxl_input_code_t code) {
	return pxl_input_state(&app.in_curr, code) == 1;
}

static inline bool
was_pressed(pxl_input_code_t code) {
	return (pxl_input_state(&app.in_prev, code) == 0) && (pxl_input_state(&app.in_curr, code) == 1);
}

static inline void
handle_input(void) {
	/* Change AI difficulty (1-5) */
	if (was_pressed(PXL_KEYB_1)) ai_set_difficulty(AI_BEGINNER);
	if (was_pressed(PXL_KEYB_2)) ai_set_difficulty(AI_EASY);
	if (was_pressed(PXL_KEYB_3)) ai_set_difficulty(AI_MEDIUM);
	if (was_pressed(PXL_KEYB_4)) ai_set_difficulty(AI_HARD);
	if (was_pressed(PXL_KEYB_5)) ai_set_difficulty(AI_IMPOSSIBLE);

	/* Speed controls */
	if (was_pressed(PXL_KEYB_6)) app.speed_factor = 0.5f;
	if (was_pressed(PXL_KEYB_7)) app.speed_factor = 1.0f;
	if (was_pressed(PXL_KEYB_8)) app.speed_factor = 2.0f;
	if (was_pressed(PXL_KEYB_9)) app.speed_factor = 4.0f;

	if (was_pressed(PXL_KEYB_T)) {
		two_players_mode = !two_players_mode;
	}

	if (was_pressed(PXL_KEYB_J) || was_pressed(PXL_KEYB_K)) {
		app.paused = false;
	}

	if (two_players_mode && (was_pressed(PXL_KEYB_Z) || was_pressed(PXL_KEYB_S))) {
		app.paused = false;
	}

	if (was_pressed(PXL_KEYB_P)) {
		app.paused = !app.paused;
	}

	bool auto_paused = is_pressed(PXL_WM_FOCUS_LOST) || is_pressed(PXL_WM_MOUSE_FOCUS_LOST);
	app.stepper.paused = app.paused || auto_paused;
}

static void
update_fps(double now, int *current_fps) {
	assert(current_fps != NULL);
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
render_pause(pxl_canvas_t *cnv) {
	assert(cnv != NULL);
	const pxl_font_t *font = pong_fonts[0];
	int scale = PAUSE_ZOOM;
	int glyph_h = font->glyph_height;

	const char pause_str[] = "PAUSE";
	int total_w = measure_text_scaled(font, pause_str, scale);
	int total_h = glyph_h * scale;

	int border = 12;
	int pad = 20;
	int x = W/2 - total_w / 2;
	int y = H/2 - total_h / 2;

	/* Outer rectangle (fg color) */
	pxl_canvas_set_color(cnv, app.palette->fg);
	pxl_fill_rect(cnv, x - pad - border, y - pad - border,
		total_w + 2 * pad + 2 * border,
		total_h + 2 * pad + 2 * border);

	/* Inner rectangle (bg color) */
	pxl_canvas_set_color(cnv, app.palette->bg);
	pxl_fill_rect(cnv, x - pad, y - pad,
		total_w + 2 * pad,
		total_h + 2 * pad);

	/* Draw "PAUSE" */
	pxl_canvas_set_color(cnv, app.palette->fg);
	draw_text_scaled(cnv, font, pause_str, scale, x, y);
}

static void
render_debug_hud(pxl_canvas_t *cnv, int fps, pxl_input_t *in) {
	assert(cnv != NULL && in != NULL);
	pxl_buf_t *pb = cnv->pb;
	int m_x = in->mouse_x, m_y = in->mouse_y;

	char hud_str[64];

	if (m_x >= 0 && m_x < pb->width && m_y >= 0 && m_y < pb->height) {
		pxl_t color = *pxl_buf_ptr(pb, m_x, m_y);
		snprintf(hud_str, sizeof(hud_str), "FPS: %d | Speed: x%.1f | AI: %s | Mouse: %d,%d | Pixel: #%06X",
			fps, app.speed_factor, ai_difficulty_names[ai_difficulty], m_x, m_y, color & 0x00FFFFFF);
	} else {
		snprintf(hud_str, sizeof(hud_str), "FPS: %d | Speed: x%.1f | AI: %s | Mouse: n/a | Pixel: n/a",
			fps, app.speed_factor, ai_difficulty_names[ai_difficulty]);
	}

	pxl_canvas_set_color(cnv, app.palette->fg);
	pxl_draw_str(cnv, 10, H - 15, hud_str);
}

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
	assert(p != NULL);
	p->ball.x = W / 2.0f;
	p->ball.y = H / 2.0f;
	p->ball.vx = (rng() % 2 == 0 ? 1.0f : -1.0f) * p->ball.speed;
	p->ball.vy = ((float)(rng() % 100) / 100.0f - 0.5f) * p->ball.speed * 1.5f;
}

static void
init_pong(pong_t *p) {
	assert(p != NULL);
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
	assert(p != NULL);
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

/* Player intent, decoupled from pong_t: -1 = up, 0 = idle, 1 = down.
 * Neither reading input nor deciding the AI move touches pong_t directly;
 * only apply_pong_input() writes into the model.
 */
typedef struct {
	float p1_dir;
	float p2_dir;
} pong_input_t;

/* Read raw key state and turn it into paddle-1 intent (always) and paddle-2
 * intent (only in 2P mode - in 1P mode paddle 2 is driven by ai_decide()
 * instead, once per fixed step, see main()).
 */
static pong_input_t
read_player_input(void) {
	pong_input_t in = {0};

	if (is_pressed(PXL_KEYB_K) || is_pressed(PXL_KEYB_UP))   in.p1_dir = -1.0f;
	if (is_pressed(PXL_KEYB_J) || is_pressed(PXL_KEYB_DOWN)) in.p1_dir =  1.0f;

	if (two_players_mode) {
		if (is_pressed(PXL_KEYB_Z)) in.p2_dir = -1.0f;
		if (is_pressed(PXL_KEYB_S)) in.p2_dir =  1.0f;
	}

	return in;
}

/* Only place that turns an intent (-1/0/1) into an actual paddle velocity. */
static void
apply_pong_input(pong_t *p, pong_input_t in) {
	assert(p != NULL);
	p->paddle_left.vy  = in.p1_dir * p->paddle_left.speed;
	p->paddle_right.vy = in.p2_dir * p->paddle_right.speed;
}

static void
interpolate_pong(pong_t *out, const pong_t *prev, const pong_t *cur, float alpha) {
	assert(out != NULL && prev != NULL && cur != NULL);
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
	assert(cnv != NULL && p != NULL);
	const pxl_font_t *font = pong_fonts[0];
	int scale = SCORE_ZOOM;
	const int score_y = 30;
	char score_str[8];

	pxl_canvas_set_color(cnv, app.palette->fg);

	/* Left score */
	snprintf(score_str, sizeof(score_str), "%d", p->score_left);
	int score_w = measure_text_scaled(font, score_str, scale);
	draw_text_scaled(cnv, font, score_str, scale, W/4 - score_w / 2, score_y);

	/* Right score */
	snprintf(score_str, sizeof(score_str), "%d", p->score_right);
	score_w = measure_text_scaled(font, score_str, scale);
	draw_text_scaled(cnv, font, score_str, scale, 3 * W / 4 - score_w / 2, score_y);
}

static void
render_pong(pxl_canvas_t *cnv, const pong_t *p) {
	assert(cnv != NULL && p != NULL);
	/* Clear */
	pxl_canvas_set_color(cnv, app.palette->bg);
	pxl_canvas_clear(cnv);

	/* Draw score */
	draw_score(cnv, p);

	/* Draw center line, paddles, ball */
	pxl_canvas_set_color(cnv, app.palette->fg);
	for (int y = 0; y < H; y += 30) {
		pxl_fill_rect(cnv, W/2 - 2, y, 4, 20);
	}

	/* Draw paddles */
	pxl_fill_rect(cnv, (int)p->paddle_left.x, (int)p->paddle_left.y,
		p->paddle_left.w, p->paddle_left.h);
	pxl_fill_rect(cnv, (int)p->paddle_right.x, (int)p->paddle_right.y,
		p->paddle_right.w, p->paddle_right.h);

	/* Draw ball */
	pxl_fill_circle(cnv, (int)p->ball.x, (int)p->ball.y, p->ball.radius);
}

static void
ai_set_difficulty(ai_difficulty_t difficulty) {
	assert(difficulty >= AI_BEGINNER && difficulty <= AI_IMPOSSIBLE);
	ai_difficulty = difficulty;
	ai.config = ai_configs[difficulty];
	/* Reset persistent error if needed */
	if (ai.config.use_persistent_error) {
		ai.current_error = ((float)(rng() % (int)(ai.config.error_range * 2)) - ai.config.error_range);
		ai.error_timer = ai.config.error_duration_min + (rng() % (ai.config.error_duration_max - ai.config.error_duration_min));
	} else {
		ai.current_error = 0.0f;
		ai.error_timer = 0;
	}
}

/* Decide the right paddle's intent (-1/0/1) for the *next* fixed step.
 * Pure decision: reads pong_t, updates only the AI's own error/timer state,
 * never writes to pong_t. The caller (main's fixed-step loop) is the one
 * that turns this into an actual paddle velocity, exactly like player
 * input does via apply_pong_input().
 */
static float
ai_decide(const pong_t *p) {
	assert(p != NULL);
	float target_y;
	const ai_config_t *c = &ai.config;

	/* Handle persistent error */
	if (c->use_persistent_error && ai.error_timer-- <= 0) {
		ai.current_error = ((float)(rng() % (int)(c->error_range * 2)) - c->error_range);
		ai.error_timer = c->error_duration_min + (rng() % (c->error_duration_max - c->error_duration_min));
	}

	/* Compute target position */
	if (p->ball.vx > 0) {
		float time_to_right = (p->paddle_right.x - p->ball.x) / p->ball.vx;
		target_y = p->ball.y + p->ball.vy * time_to_right + c->prediction_bias;
		/* Add persistent error if active */
		if (c->use_persistent_error) {
			target_y += ai.current_error;
		}
	} else {
		target_y = H / 2.0f;
	}

	/* Clamp to playable area */
	target_y = fmaxf(p->paddle_right.h / 2.0f,
		fminf(H - p->paddle_right.h / 2.0f, target_y));

	/* Calculate required movement */
	float error = target_y - (p->paddle_right.y + p->paddle_right.h / 2.0f);

	if (fabsf(error) > c->reaction_threshold) {
		return (error < 0 ? -1.0f : 1.0f);
	}
	return 0.0f;
}

int
main(void) {
	printf("Pong game. Vim keys: J=down, K=up. Player 2: Z=up, S=down. T=2P mode. 6-9=speed, 1-5=AI difficulty, CTRL=HUD, P=pause, ESC=quit\n");

	if (pxl_backend_init("PXL Pong", W, H, PXL_BACKEND_CENTERED) != PXL_SUCCESS)
		return 1;

	app.current_fps = 0;
	app.stepper.dt = 1.0f / FPS;
	app.speed_factor = 1.0f;
	pxl_stepper_init(&app.stepper, pxl_backend_get_time());

	/* Initialize palettes */
	palette_paused = (palette_t){
		.fg = color_grayscale(palette_normal.fg),
		.bg = color_grayscale(palette_normal.bg)
	};
	app.palette = &palette_normal;

	/* Initialize AI */
	ai_set_difficulty(AI_MEDIUM);

	pong_t pong;
	init_pong(&pong);
	pong_t pong_prev = pong;

	while (!is_pressed(PXL_KEYB_ESCAPE) && !is_pressed(PXL_WM_QUIT)) {
		app.in_prev = app.in_curr;
		pxl_backend_poll_events(&app.in_curr);

		handle_input();

		/* Player intent, read once per frame; applied to the model right
		 * away for the left paddle (and the right paddle too in 2P mode).
		 * In 1P mode p2_dir stays 0 here - the AI drives it below, once
		 * per fixed step since it needs to react to the ball's position
		 * as it evolves across steps, not just once per rendered frame.
		 */
		pong_input_t pinput = read_player_input();
		apply_pong_input(&pong, pinput);

		/* Update palette based on pause state */
		app.palette = app.stepper.paused ? &palette_paused : &palette_normal;

		pxl_stepper_sync_time(&app.stepper, pxl_backend_get_time());

		while (pxl_stepper_advance(&app.stepper)) {
			if (!two_players_mode) {
				pong.paddle_right.vy = ai_decide(&pong) * pong.paddle_right.speed;
			}

			pong_prev = pong;
			update_pong(&pong, (float)app.stepper.dt * app.speed_factor);
		}

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			pong_t pong_interpolated;
			interpolate_pong(&pong_interpolated, &pong_prev, &pong, app.stepper.lerp_factor);
			render_pong(&cnv, &pong_interpolated);

			/* Show HUD when CTRL is pressed */
			if (is_pressed(PXL_KEYB_LCTRL) || is_pressed(PXL_KEYB_RCTRL)) {
				render_debug_hud(&cnv, app.current_fps, &app.in_curr);
			}

			/* Draw pause overlay */
			if (app.stepper.paused) {
				render_pause(&cnv);
			}

			pxl_backend_end_frame();
		}

		update_fps(pxl_backend_get_time(), &app.current_fps);
	}

	printf("Final Score: %d - %d (AI: %s)\n", pong.score_left, pong.score_right, ai_difficulty_names[ai_difficulty]);
	pxl_backend_deinit();
	return 0;
}
