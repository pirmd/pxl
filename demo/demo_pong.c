/*
 * PXL Demo: Pong Game (1P vs AI)
 *
 * Shows PXL core features:
 *   - Window/event loop (pxl_app_init/advance/deinit)
 *   - Fixed-timestep physics (via pxl_app_advance_physics)
 *   - Canvas-based rendering with scissor regions
 *   - Input handling
 *   - Time-based interpolation for smooth rendering
 *   - Bitmap font writing
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "pxl.h"
#include "font_9x15.h"

/* UI and LAYOUT */
#define W 800
#define H 600

#define PAUSE_W    160
#define PAUSE_H    120

#define HELP_W     400 
#define HELP_H     300

#define SCORE_H    50

#define FONT_ZOOM  2

/* Color */
#define FG_COLOR  0xFFFFFFFFU
#define BG_COLOR  0xFF0000FFU

/* RNG --------------------------------------------------------------------- */
static inline uint32_t *rng_state_ptr(void) {
	static uint32_t state = 0;
	return &state;
}

static inline void
rng_seed(uint32_t seed) {
	*rng_state_ptr() = seed ? seed : (uint32_t)time(NULL);
}

static inline uint32_t
rng(void) {
	uint32_t *state = rng_state_ptr();
	*state = *state * 1664525u + 1013904223u;
	return *state;
}

/* UI state ----------------------------------------------------------------- */
typedef struct {
	const pxl_font_t **fonts;
	size_t font_count;
	
	int  font_scale;
	bool show_pause;
	bool show_help;
} ui_t;

static void
ui_draw_text(const ui_t *ui, pxl_canvas_t *cnv, const char *txt, pxl_align_t align) {
	assert(cnv && ui && txt);
	
	pxl_rect_t bbox = pxl_canvas_view(cnv);

	pxl_writer_t w;
	pxl_writer_init(&w, ui->fonts, ui->font_count);

	pxl_rect_t bounds = pxl_text_bounds_transformed(&w, txt, ui->font_scale);
	pxl_rect_t aligned = pxl_align_rect(bounds, bbox, align);

	pxl_writer_set_cursor(&w, aligned.x, aligned.y);
	pxl_draw_text_transformed(cnv, &w, txt, ui->font_scale, PXL_FLIP_NONE);
}

/* Geometric layout -------------------------------------------------------- */
typedef struct {
	pxl_rect_t root;
	pxl_rect_t score;
	pxl_rect_t arena;
	pxl_rect_t pause;
	pxl_rect_t help;
} layout_t;

static layout_t
compute_layout(void) {
	layout_t l = {0};

	pxl_backend_get_window_size(&l.root.w, &l.root.h);

	l.arena = l.root;
	l.score = pxl_split_rect(&l.arena, SCORE_H, PXL_SIDE_TOP);

	l.pause = pxl_align_rect((pxl_rect_t){0, 0, PAUSE_W, PAUSE_H}, l.root, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
	l.help  = pxl_align_rect((pxl_rect_t){0, 0, HELP_W, HELP_H}, l.root, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
	
	return l;
}

/* Pong game --------------------------------------------------------------- */
typedef struct {
	pxl_rect_t arena;

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
ball_reset(pong_t *p) {
	assert(p);
	p->ball.x = (float)p->arena.x + (float)p->arena.w / 2.0f;
	p->ball.y = (float)p->arena.y + (float)p->arena.h / 2.0f;
	p->ball.vx = (rng() % 2 == 0 ? 1.0f : -1.0f) * p->ball.speed;
	p->ball.vy = ((float)(rng() % 100) / 100.0f - 0.5f) * p->ball.speed * 1.5f;
}

static void
init_pong(pong_t *p, pxl_rect_t arena) {
	assert(p);
	
	p->arena = arena;

	/* Paddles */
	p->paddle_left.x = 20.0f;
	p->paddle_left.y = (float)p->arena.h/2.0f - 50.0f;
	p->paddle_left.w = 15.0f;
	p->paddle_left.h = 100.0f;
	p->paddle_left.speed = 400.0f;
	p->paddle_left.vy = 0.0f;

	p->paddle_right.x = (float)p->arena.w - 20.0f - 15.0f;
	p->paddle_right.y = (float)p->arena.h/2.0f - 50.0f;
	p->paddle_right.w = 15.0f;
	p->paddle_right.h = 100.0f;
	p->paddle_right.speed = 400.0f;
	p->paddle_right.vy = 0.0f;

	/* Ball */
	p->ball.radius = 8.0f;
	p->ball.speed = 300.0f;
	ball_reset(p);

	/* Scores */
	p->score_left = 0;
	p->score_right = 0;
}

typedef struct {
	int paddle_left_dir;
	int paddle_right_dir;
} pong_input_t;

static void
update_pong(const pong_t *cur, float dt, pong_input_t input, pong_t *next) {
	assert(next && cur);
	*next = *cur;

	/* Apply input to velocities */
	next->paddle_left.vy  = (float)input.paddle_left_dir * next->paddle_left.speed;
	next->paddle_right.vy = (float)input.paddle_right_dir * next->paddle_right.speed;

	/* Update paddles */
	next->paddle_left.y  += next->paddle_left.vy * dt;
	next->paddle_right.y += next->paddle_right.vy * dt;

	/* Clamp paddles to screen */
	if (next->paddle_left.y < (float)next->arena.y) next->paddle_left.y = (float)next->arena.y;
	if (next->paddle_left.y + next->paddle_left.h > (float)(next->arena.y + next->arena.h)) 
		next->paddle_left.y = (float)(next->arena.y + next->arena.h) - next->paddle_left.h;
	
	if (next->paddle_right.y < (float)next->arena.y) next->paddle_right.y = (float)next->arena.y;
	if (next->paddle_right.y + next->paddle_right.h > (float)(next->arena.y + next->arena.h)) 
		next->paddle_right.y = (float)(next->arena.y + next->arena.h) - next->paddle_right.h;

	/* Update ball */
	next->ball.x += next->ball.vx * dt;
	next->ball.y += next->ball.vy * dt;

	/* Ball collision with top and bottom */
	if (next->ball.y - next->ball.radius < (float)next->arena.y) {
		next->ball.y = (float)next->arena.y + next->ball.radius;
		next->ball.vy = -next->ball.vy;
	}
	if (next->ball.y + next->ball.radius > (float)(next->arena.y + next->arena.h)) {
		next->ball.y = (float)(next->arena.y + next->arena.h) - next->ball.radius;
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
	if (next->ball.x - next->ball.radius < (float)next->arena.x) {
		next->score_right++;
		ball_reset(next);
	} else if (next->ball.x + next->ball.radius > (float)(next->arena.x + next->arena.w)) {
		next->score_left++;
		ball_reset(next);
	}
}

static void
interpolate_pong(const pong_t *prev, const pong_t *cur, float alpha, pong_t *out) {
	assert(out && prev && cur);
	out->arena = cur->arena;

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

/* Input ------------------------------------------------------------------- */
static void
handle_ai_input(const pong_t *p, pong_input_t *input) {
	assert(input && p);

	float target_y = p->ball.y;
	float error = target_y - (p->paddle_right.y + p->paddle_right.h / 2.0f);
	input->paddle_right_dir = (fabsf(error) > 5.0f) ? (error < 0 ? -1 : 1) : 0;
}

static void
handle_player_input(pxl_app_t *app, pong_input_t *input) {
	assert(app && input);

	if (pxl_app_is_active(app, PXL_KEYB_K) || pxl_app_is_active(app, PXL_KEYB_UP))
		input->paddle_left_dir = -1;

	if (pxl_app_is_active(app, PXL_KEYB_J) || pxl_app_is_active(app, PXL_KEYB_DOWN))
		input->paddle_left_dir = 1;
}

static void
handle_ui_input(pxl_app_t *app, ui_t *ui) {
	/* Pause */
	if (pxl_app_is_active(app, PXL_WM_FOCUS_LOST) ||
	    pxl_app_is_active(app, PXL_WM_MOUSE_FOCUS_LOST)) {
		ui->show_pause = true;
	}

	if (pxl_app_is_active(app, PXL_KEYB_J) || pxl_app_is_active(app, PXL_KEYB_K) ||
	    pxl_app_is_active(app, PXL_KEYB_UP) || pxl_app_is_active(app, PXL_KEYB_DOWN)) {
		ui->show_pause = false;
	}

	if (pxl_app_was_triggered(app, PXL_KEYB_P)) {
		ui->show_pause = !ui->show_pause;
	}

	/* Help screen toggle */
	ui->show_help = pxl_app_is_active(app, PXL_KEYB_H);

	app->paused = ui->show_pause || ui->show_help;
}

/* Render ------------------------------------------------------------------ */
static void
render_game(pxl_canvas_t *cnv, const pong_t *p) {
	assert(cnv && p);

	pxl_fill_rect(cnv, (int)(p->paddle_left.x - (float)p->arena.x), (int)(p->paddle_left.y - (float)p->arena.y),
		(int)p->paddle_left.w, (int)p->paddle_left.h);

	pxl_fill_rect(cnv, (int)(p->paddle_right.x - (float)p->arena.x), (int)(p->paddle_right.y - (float)p->arena.y),
		(int)p->paddle_right.w, (int)p->paddle_right.h);

	pxl_fill_circle(cnv, (int)(p->ball.x - (float)p->arena.x), (int)(p->ball.y - (float)p->arena.y), (int)p->ball.radius);
}

static void
render_score(pxl_canvas_t *cnv, const pong_t *p, const ui_t *ui) {
	assert(cnv && ui && p);
	
	char txt[16];
	snprintf(txt, sizeof(txt), "%d : %d", p->score_left, p->score_right);

	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
}

static void
outline_rect(pxl_canvas_t *cnv, pxl_rect_t r, int thick) {
	pxl_fill_rect(cnv, r.x, r.y, r.w, thick);
	pxl_fill_rect(cnv, r.x, r.y, thick, r.h);
	pxl_fill_rect(cnv, r.x + r.w - thick, r.y, thick, r.h);
	pxl_fill_rect(cnv, r.x, r.y + r.h - thick, r.w, thick);
}

static void
render_pause(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv && ui);

	const char *txt = "PAUSE";
	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
	outline_rect(cnv, pxl_canvas_view(cnv), 2 * ui->font_scale);
}

static void
render_help(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv && ui);

	const char *txt =
		"  ~ CONTROLS ~\n"
		"\n"
		"K/J: move paddle\n"
		"P  : pause\n"
		"H  : help\n"
		"ESC: quit";

	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
	outline_rect(cnv, pxl_canvas_view(cnv), 2 * ui->font_scale);
}

static inline void
render_fps(pxl_canvas_t *cnv, const ui_t *ui, int fps) {
	assert(cnv);
	assert(ui);

	char txt[16];
	snprintf(txt, sizeof(txt), "FPS: %d", fps);
	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_RIGHT|PXL_ALIGN_BOTTOM);
}

/* FPS counter ------------------------------------------------------------- */
static inline void
update_fps(double frame_dt, int *current_fps) {
	assert(current_fps);

	static double accumulator = 0;
	static int frame_count = 0;
	accumulator += frame_dt;
	frame_count++;
	if (accumulator >= 1.0) {
		*current_fps = (int)((float)frame_count / accumulator);
		frame_count = 0;
		accumulator = 0;
	}
}

/* Main -------------------------------------------------------------------- */
int
main(void) {
	pxl_app_cfg_t cfg = {
		.title = "PXL Pong",
		.width = W, .height = H,
		.physics_dt = 1.0 / 60.0,
	};

	pxl_app_t app;
	if (pxl_app_init(&app, &cfg) != PXL_SUCCESS) return 1;

	rng_seed(0);

	ui_t ui = {
		.fonts      = (const pxl_font_t *[]){ &font_9x15_latin },
		.font_count = 1,
		.font_scale = FONT_ZOOM
	};

	layout_t layout = compute_layout();

	pong_t pong, pong_prev;
	init_pong(&pong, layout.arena);

	int fps = 0;

	while (pxl_app_advance(&app)) {
		if (pxl_app_was_triggered(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		handle_ui_input(&app, &ui);

		pong_input_t pong_input = {0};
		handle_player_input(&app, &pong_input);
		handle_ai_input(&pong, &pong_input);

		while (pxl_app_advance_physics(&app)) {
			pong_prev = pong;
			update_pong(&pong_prev, (float)app.physics_ts.dt, pong_input, &pong);
		}

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			/* Setup view for each area.*/
			pxl_canvas_t cnv, cnv_score, cnv_arena, cnv_pause, cnv_help;			
			pxl_canvas_init(&cnv, &pb);
			pxl_canvas_init_view(&cnv_score, &pb, layout.score);
			pxl_canvas_init_view(&cnv_arena, &pb, layout.arena);
			pxl_canvas_init_view(&cnv_pause, &pb, layout.pause);
			pxl_canvas_init_view(&cnv_help,  &pb, layout.help);

			/* Clear */
			pxl_canvas_set_color(&cnv, BG_COLOR);
			pxl_canvas_clear(&cnv);

			/* Smooth rendering of game arena */
			pong_t pong_interpolated;
			interpolate_pong(&pong_prev, &pong, app.physics_ts.alpha, &pong_interpolated);
			render_game(&cnv_arena, &pong_interpolated);
			
			/* Draw score and fps */
			render_score(&cnv_score, &pong, &ui);
			render_fps(&cnv, &ui, fps);

			/* Draw pause overlay */
			if (ui.show_pause) {
				pxl_canvas_set_color(&cnv_pause, BG_COLOR);
				pxl_canvas_clear(&cnv_pause);
				render_pause(&cnv_pause, &ui);
			}

			/* Draw help overlay */
			if (ui.show_help) {
				pxl_canvas_set_color(&cnv_help, BG_COLOR);
				pxl_canvas_clear(&cnv_help);
				render_help(&cnv_help, &ui);
			}

			(void)pxl_backend_end_frame();
		}

		update_fps(app.frame_dt, &fps);
	}

	pxl_app_deinit(&app);
	return 0;
}
