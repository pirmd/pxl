/*
 * PXL Demo: Pong Game
 *
 * This is a COMPLETE game demo showcasing PXL's core features:
 *   - Window and event loop (pxl_app_init/advance/deinit)
 *   - Fixed-timestep physics (stepper.h via pxl_app_advance_physics)
 *   - Canvas-based rendering with scissor regions
 *   - Input handling (is_pressed vs was_pressed)
 *   - Time-based interpolation for smooth rendering at any framerate
 *
 * Architecture:
 *   Game state is updated in fixed timesteps (update_pong),
 *   then interpolated for rendering (interpolate_pong).
 *
 * To use as a boilerplate:
 *   1. Copy the main() structure
 *   2. Replace pong_t with your game state
 *   3. Keep pxl_* calls and stepper pattern
 *
 * Note: demo_helpers.h contains demo-specific utilities (NOT part of PXL core).
 *       font_9x15.h is a generated font (see tool/bdf2pxl).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "pxl.h"              /* Core PXL library (includes all public headers) */
#include "demo_helpers.h"     /* Demo-specific: demo_rng(), demo_draw_text_scaled(), demo_text_bounds_scaled() */
#include "font_9x15.h"        /* Auto-generated font header (see tool/bdf2pxl) */

#define W 800
#define H 600
#define Wf 800.0f
#define Hf 600.0f
#define FPS 60.0f

/* UI */
#define SCORE_ZOOM  3
#define PAUSE_ZOOM  4

#define FG_COLOR 0xFFFFFFFF
#define BG_COLOR 0xFF000080

typedef enum {
	GAME_1P,
	GAME_2P
} game_mode_t;

typedef struct {
	const pxl_font_t *font;
	game_mode_t mode;
	bool show_pause;
	bool show_help;
} ui_t;

/* Pong game */

typedef struct {
	int paddle_left_dir;
	int paddle_right_dir;
} pong_input_t;

typedef struct {
	struct {
		float x, y;
		float w, h;
		float speed;
		float vy;
	} paddle_left, paddle_right;

	struct {
		float x, y;
		float radius;
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
	p->ball.vx = (demo_rng() % 2 == 0 ? 1.0f : -1.0f) * p->ball.speed;
	p->ball.vy = ((float)(demo_rng() % 100) / 100.0f - 0.5f) * p->ball.speed * 1.5f;
}

static void
init_pong(pong_t *p) {
	assert(p != NULL);
	/* Paddles */
	p->paddle_left.x = 20.0f;
	p->paddle_left.y = (float)(H/2 - 50);
	p->paddle_left.w = 15.0f;
	p->paddle_left.h = 100.0f;
	p->paddle_left.speed = 400.0f;
	p->paddle_left.vy = 0.0f;

	p->paddle_right.x = (float)(W - 20 - 15);
	p->paddle_right.y = (float)(H/2 - 50);
	p->paddle_right.w = 15.0f;
	p->paddle_right.h = 100.0f;
	p->paddle_right.speed = 400.0f;
	p->paddle_right.vy = 0.0f;

	/* Ball */
	p->ball.radius = 8.0f;
	p->ball.speed = 300.0f;
	reset_ball(p);

	/* Scores */
	p->score_left = 0;
	p->score_right = 0;
}

static void
interpolate_pong(const pong_t *prev, const pong_t *cur, float alpha, pong_t *out) {
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
update_pong(const pong_t *current, float dt, pong_input_t input, pong_t *next) {
	assert(next != NULL && current != NULL);
	*next = *current;

	/* Apply input to velocities */
	next->paddle_left.vy  = (float)input.paddle_left_dir * next->paddle_left.speed;
	next->paddle_right.vy = (float)input.paddle_right_dir * next->paddle_right.speed;

	/* Update paddles */
	next->paddle_left.y  += next->paddle_left.vy * dt;
	next->paddle_right.y += next->paddle_right.vy * dt;

	/* Clamp paddles to screen */
	if (next->paddle_left.y < 0.0f) next->paddle_left.y = 0.0f;
	if (next->paddle_left.y + next->paddle_left.h > Hf) next->paddle_left.y = Hf - next->paddle_left.h;
	if (next->paddle_right.y < 0.0f) next->paddle_right.y = 0.0f;
	if (next->paddle_right.y + next->paddle_right.h > Hf) next->paddle_right.y = Hf - next->paddle_right.h;

	/* Update ball */
	next->ball.x += next->ball.vx * dt;
	next->ball.y += next->ball.vy * dt;

	/* Ball collision with top and bottom */
	if (next->ball.y - next->ball.radius < 0.0f) {
		next->ball.y = next->ball.radius;
		next->ball.vy = -next->ball.vy;
	}
	if (next->ball.y + next->ball.radius > Hf) {
		next->ball.y = Hf - next->ball.radius;
		next->ball.vy = -next->ball.vy;
	}

	/* Ball collision with paddles */
	/* Left paddle */
	if (next->ball.x - next->ball.radius < next->paddle_left.x + next->paddle_left.w &&
	    next->ball.y + next->ball.radius > next->paddle_left.y &&
	    next->ball.y - next->ball.radius < next->paddle_left.y + next->paddle_left.h) {
		next->ball.x = next->paddle_left.x + next->paddle_left.w + next->ball.radius;
		next->ball.vx = -next->ball.vx * 1.05f;
		float paddle_center = next->paddle_left.y + next->paddle_left.h / 2.0f;
		float hit_pos = (next->ball.y - paddle_center) / (next->paddle_left.h / 2.0f);
		next->ball.vy = hit_pos * next->ball.speed * 0.8f;
	}

	/* Right paddle */
	if (next->ball.x + next->ball.radius > next->paddle_right.x &&
	    next->ball.y + next->ball.radius > next->paddle_right.y &&
	    next->ball.y - next->ball.radius < next->paddle_right.y + next->paddle_right.h) {
		next->ball.x = next->paddle_right.x - next->ball.radius;
		next->ball.vx = -next->ball.vx * 1.05f;
		float paddle_center = next->paddle_right.y + next->paddle_right.h / 2.0f;
		float hit_pos = (next->ball.y - paddle_center) / (next->paddle_right.h / 2.0f);
		next->ball.vy = hit_pos * next->ball.speed * 0.8f;
	}

	/* Ball out of bounds (score) */
	if (next->ball.x - next->ball.radius < 0.0f) {
		next->score_right++;
		reset_ball(next);
	}
	if (next->ball.x + next->ball.radius > Wf) {
		next->score_left++;
		reset_ball(next);
	}
}

/* AI */
static void
ai_get_input(pong_input_t *input, const pong_t *p) {
	assert(input != NULL && p != NULL);

	/* Simple AI: follow the ball with right paddle */
	float target_y = p->ball.y;
	float error = target_y - (p->paddle_right.y + p->paddle_right.h / 2.0f);
	input->paddle_right_dir = (fabsf(error) > 5.0f) ? (error < 0 ? -1 : 1) : 0;
}

/* Inputs */
static void
p1_get_input(pxl_app_t *app, pong_input_t *input) {
	assert(app != NULL && input != NULL);
	input->paddle_left_dir = 0;
	if (pxl_app_is_pressed(app, PXL_KEYB_K) || pxl_app_is_pressed(app, PXL_KEYB_UP))
		input->paddle_left_dir = -1;
	if (pxl_app_is_pressed(app, PXL_KEYB_J) || pxl_app_is_pressed(app, PXL_KEYB_DOWN))
		input->paddle_left_dir = 1;
}

static void
p2_get_input(pxl_app_t *app, pong_input_t *input) {
	assert(app != NULL && input != NULL);
	input->paddle_right_dir = 0;
	if (pxl_app_is_pressed(app, PXL_KEYB_Z)) input->paddle_right_dir = -1;
	if (pxl_app_is_pressed(app, PXL_KEYB_S)) input->paddle_right_dir = 1;
}

static void
handle_pong_input(pxl_app_t *app, const ui_t *ui, const pong_t *pong, pong_input_t *input) {
	assert(app != NULL && ui != NULL && pong != NULL && input != NULL);
	*input = (pong_input_t){0};

	if (ui->show_pause) return;

	switch (ui->mode) {
	case GAME_1P:
		p1_get_input(app, input);
		ai_get_input(input, pong);  /* AI controls right paddle */
		break;
	case GAME_2P:
		p1_get_input(app, input);
		p2_get_input(app, input);
		break;
	}
}

static void
handle_input(pxl_app_t *app, ui_t *ui) {
	/* Cycle through game modes: 1 player <-> 2 players */
	if (pxl_app_was_pressed(app, PXL_KEYB_T)) {
		ui->mode = (ui->mode == GAME_1P) ? GAME_2P : GAME_1P;
	}

	/* Focus-based pause: auto-pause on focus loss */
	if (pxl_app_is_pressed(app, PXL_WM_FOCUS_LOST) ||
	    pxl_app_is_pressed(app, PXL_WM_MOUSE_FOCUS_LOST)) {
		ui->show_pause = true;
	}

	/* Manual unpause: movement keys clear pause */
	if (pxl_app_is_pressed(app, PXL_KEYB_J) || pxl_app_is_pressed(app, PXL_KEYB_K) ||
	    pxl_app_is_pressed(app, PXL_KEYB_Z) || pxl_app_is_pressed(app, PXL_KEYB_S)) {
		ui->show_pause = false;
	}

	/* Manual pause toggle */
	if (pxl_app_was_pressed(app, PXL_KEYB_P)) {
		ui->show_pause = !ui->show_pause;
	}

	/* Help screen toggle */
	ui->show_help = pxl_app_is_pressed(app, PXL_KEYB_H);

	app->physics_ts.paused = ui->show_pause || ui->show_help;
}

/* Render */
static void
render_score(pxl_canvas_t *cnv, const pong_t *p, const ui_t *ui) {
	assert(cnv != NULL && p != NULL && ui != NULL);
	const pxl_font_t *font = ui->font;
	int scale = SCORE_ZOOM;
	const int viewport_w = cnv->scissor.w;
	const int viewport_h = cnv->scissor.h;

	pxl_canvas_set_color(cnv, FG_COLOR);

	/* Left score */
	char score_str[8];
	snprintf(score_str, sizeof(score_str), "%d", p->score_left);
	pxl_rect_t bounds = demo_text_bounds_scaled(font, score_str, scale);
	demo_draw_text_scaled(cnv, font, score_str, scale,
			viewport_w / 4 - bounds.w / 2,
			viewport_h / 2 - bounds.h / 2);

	/* Right score */
	snprintf(score_str, sizeof(score_str), "%d", p->score_right);
	bounds = demo_text_bounds_scaled(font, score_str, scale);
	demo_draw_text_scaled(cnv, font, score_str, scale,
			3 * viewport_w / 4 - bounds.w / 2,
			viewport_h / 2 - bounds.h / 2);
}

static void
render_game(pxl_canvas_t *cnv, const pong_t *p, const ui_t *ui) {
	assert(cnv != NULL && p != NULL && ui != NULL);

	const int viewport_h = cnv->scissor.h;

	/* Draw center line */
	pxl_canvas_set_color(cnv, FG_COLOR);
	for (int y = 0; y < viewport_h; y += 30) {
		pxl_fill_rect(cnv, cnv->scissor.w / 2 - 2, y, 4, 20);
	}

	/* Draw paddles */
	pxl_fill_rect(cnv, (int)p->paddle_left.x, (int)p->paddle_left.y,
		(int)p->paddle_left.w, (int)p->paddle_left.h);
	pxl_fill_rect(cnv, (int)p->paddle_right.x, (int)p->paddle_right.y,
		(int)p->paddle_right.w, (int)p->paddle_right.h);

	/* Draw ball */
	pxl_fill_circle(cnv, (int)p->ball.x, (int)p->ball.y, (int)p->ball.radius);
}

static void
render_pause(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);
	const pxl_font_t *font = ui->font;
	int scale = PAUSE_ZOOM;

	const char pause_str[] = "PAUSE";
	pxl_rect_t bounds = demo_text_bounds_scaled(font, pause_str, scale);

	int border = bounds.h / 6;
	int pad = bounds.h / 2;
	int x = W/2 - bounds.w / 2;
	int y = H/2 - bounds.h / 2;

	uint32_t fg = FG_COLOR;
	uint32_t bg = BG_COLOR;

	/* Outer rectangle (fg color) */
	pxl_canvas_set_color(cnv, fg);
	pxl_fill_rect(cnv, x - pad - border, y - pad - border,
		bounds.w + 2 * pad + 2 * border,
		bounds.h + 2 * pad + 2 * border);

	/* Inner rectangle (bg color) */
	pxl_canvas_set_color(cnv, bg);
	pxl_fill_rect(cnv, x - pad, y - pad,
		bounds.w + 2 * pad,
		bounds.h + 2 * pad);

	/* Draw "PAUSE" */
	pxl_canvas_set_color(cnv, fg);
	demo_draw_text_scaled(cnv, font, pause_str, scale, x, y);
}

static void
render_help(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);
	const pxl_font_t *font = ui->font;
	int scale = 2;

	const char *help_lines[] = {
		"CONTROLS:",
		"",
		"P1: K/Up, J/Down",
		"P2: Z, S",
		"",
		"T: toggle 1P/2P",
		"P: pause",
		"H: help",
		"ESC: quit"
	};
	int line_count = sizeof(help_lines) / sizeof(help_lines[0]);

	/* Calculate total bounds */
	int max_width = 0;
	int total_height = 0;
	for (int i = 0; i < line_count; i++) {
		pxl_rect_t bounds = demo_text_bounds_scaled(font, help_lines[i], scale);
		if (bounds.w > max_width) max_width = bounds.w;
		total_height += bounds.h;
	}

	int border = scale * 4;
	int pad = scale * 6;
	int x = (W - max_width) / 2 - pad - border;
	int y = (H - total_height) / 2 - pad - border;

	uint32_t fg = FG_COLOR;
	uint32_t bg = BG_COLOR;

	/* Outer rectangle (fg color) */
	pxl_canvas_set_color(cnv, fg);
	pxl_fill_rect(cnv, x, y,
		max_width + 2 * pad + 2 * border,
		total_height + 2 * pad + 2 * border);

	/* Inner rectangle (bg color) */
	pxl_canvas_set_color(cnv, bg);
	pxl_fill_rect(cnv, x + border, y + border,
		max_width + 2 * pad,
		total_height + 2 * pad);

	/* Draw help lines */
	pxl_canvas_set_color(cnv, fg);
	int current_y = y + border + pad;
	for (int i = 0; i < line_count; i++) {
		pxl_rect_t bounds = demo_text_bounds_scaled(font, help_lines[i], scale);
		int line_x = x + border + pad + (max_width - bounds.w) / 2;
		demo_draw_text_scaled(cnv, font, help_lines[i], scale, line_x, current_y);
		current_y += bounds.h;
	}
}

int
main(void) {
	printf("Pong game.\n"
		"Player 1: UP/DOWN or K/J=up/down. Player 2: Z/S=up/down.\n"
		"T=toggle 1P/2P, P=pause, H=help, ESC=quit\n");

	pxl_app_t app = {
		.title = "PXL Pong",
		.width = W,
		.height = H,
		.physics_hz = FPS,
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	ui_t ui = {
		.font = &font_9x15_latin,
		.mode = GAME_1P
	};

	/* Two states:
	 * - pong: current state (updated by physics)
	 * - pong_prev: previous state (for interpolation)
	 * This avoids "stuttering" when frame rate != physics rate.
	 */
	pong_t pong;
	init_pong(&pong);
	pong_t pong_prev = pong;

	int fps = 0;

	/* Main loop with fixed-timestep physics:
	 *
	 * pxl_app_advance()      - Advances frame timer, processes OS events
	 * pxl_app_advance_physics()- Runs physics at fixed rate (FPS Hz)
	 * app.physics_ts.dt      - Fixed delta time (1/FPS seconds)
	 * app.physics_ts.lerp_factor - Blend factor for interpolation [0,1)
	 *
	 * This ensures deterministic physics regardless of frame rate.
	 */
	while (pxl_app_advance(&app)) {
		if (pxl_app_was_pressed(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		handle_input(&app, &ui);

		pong_input_t pong_input;
		handle_pong_input(&app, &ui, &pong, &pong_input);

		while (pxl_app_advance_physics(&app)) {
			pong_prev = pong;
			update_pong(&pong_prev, (float)app.physics_ts.dt, pong_input, &pong);
		}

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {

			/* Setup viewports for each area */
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			pxl_canvas_t cnv_score = cnv;
			pxl_canvas_set_scissor(&cnv_score, 0, 0, W, 50);

			pxl_canvas_t cnv_game = cnv;
			pxl_canvas_set_scissor(&cnv_game, 0, 0, W, H);

			/* Clear */
			pxl_canvas_set_color(&cnv, BG_COLOR);
			pxl_canvas_clear(&cnv);

			/* Smooth rendering via interpolation:
			 * Blends pong_prev and pong using lerp_factor.
			 * At 60 FPS with 60 physics steps: lerp_factor = 0 (no blend).
			 * At 120 FPS with 60 physics steps: lerp_factor = 0.5 (midpoint).
			 */
			pong_t pong_interpolated;
			interpolate_pong(&pong_prev, &pong, app.physics_ts.lerp_factor, &pong_interpolated);

			/* Draw score and game areas */
			render_score(&cnv_score, &pong_interpolated, &ui);
			render_game(&cnv_game, &pong_interpolated, &ui);

			/* Draw FPS in bottom right corner */
			if (fps > 0) {
				char fps_str[16];
				snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
				pxl_rect_t fps_bounds = demo_text_bounds_scaled(ui.font, fps_str, 1);
				pxl_canvas_set_color(&cnv_game, FG_COLOR);
				demo_draw_text_scaled(&cnv_game, ui.font, fps_str, 1,
					W - fps_bounds.w - 10, H - fps_bounds.h - 10);
			}

			/* Draw pause overlay */
			if (ui.show_pause) {
				render_pause(&cnv, &ui);
			}

			/* Draw help overlay */
			if (ui.show_help) {
				render_help(&cnv, &ui);
			}

			(void)pxl_backend_end_frame();
		}

		 demo_update_fps(pxl_backend_get_time(), &fps);
	}

	pxl_app_deinit(&app);
	return 0;
}
