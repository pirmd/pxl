#ifndef PXL_APP_H
#define PXL_APP_H

#include <assert.h>
#include <stdbool.h>

#include "err.h"
#include "backend.h"
#include "input.h"
#include "stepper.h"

/* Maximum frame time to avoid spiral of death on frame drops. */
#define PXL_APP_MAX_FRAME_TIME 0.25

/*
 * PXL App Layer - Frame management, input transitions, and time control.
 *
 * Always call pxl_app_advance() every frame, even when paused.
 *
 * Usage:
 *   pxl_app_cfg_t cfg = {
 *       .title = "My Game",
 *       .width = 800,
 *       .height = 600,
 *       .physics_dt = 1.0 / 60.0,  // Fixed timestep for physics
 *   };
 *   pxl_app_t app;
 *   pxl_app_init(&app, &cfg);
 *
 *   while (pxl_app_advance(&app)) {
 *       if (pxl_app_was_triggered(&app, PXL_KEY_P)) app.paused = !app.paused;
 *       if (pxl_app_was_triggered(&app, PXL_KEY_LEFT_SHIFT)) app.time_scale = 0.5f;
 *
 *       // Use pxl_app_dt(&app) for time-aware updates:
 *       pxl_timer_advance(&my_timer, pxl_app_dt(&app));
 *
 *       // Physics updates:
 *       while (pxl_app_advance_physics(&app)) update_physics();
 *       render();
 *   }
 *   pxl_app_deinit(&app);
 */

typedef struct {
	const char* title;        /* Window title. */
	int width, height;        /* Window dimensions. */
	pxl_backend_flags_t backend_flags;  /* Backend-specific flags. */
	double physics_dt;        /* Fixed timestep for physics (0 = disable). */
} pxl_app_cfg_t;

typedef struct {
	/* Time control */
	float time_scale;    /* Global time scale (default: 1.0). */
	bool paused;          /* Global pause flag. */

	/* Internal state */
	pxl_input_t curr;    /* Current input state. */
	pxl_input_t prev;    /* Previous input state. */

	pxl_time_stepper_t physics_ts; /* Physics stepper state. */

	/* Time tracking */
	double prev_time;    /* Time at previous frame. */
	double frame_dt;     /* Raw frame delta time (clamped). */
} pxl_app_t;

/* Returns effective dt (paused-aware, time-scaled). Use for game logic. */
static inline double pxl_app_dt(const pxl_app_t *app) {
	return app->paused ? 0.0 : (app->frame_dt * (double)app->time_scale);
}

/* Initialize app and backend from configuration. */
static inline pxl_err_t
pxl_app_init(pxl_app_t *app, const pxl_app_cfg_t *cfg) {
	assert(app);
	assert(cfg);

	if (pxl_backend_init(cfg->title, cfg->width, cfg->height, cfg->backend_flags) != PXL_SUCCESS) {
		return PXL_E_BACKEND_INIT;
	}

	app->curr = (pxl_input_t){0};
	app->prev = (pxl_input_t){0};

	app->time_scale = 1.0f;
	app->paused = false;
	app->prev_time = pxl_backend_get_time();

	pxl_stepper_init(&app->physics_ts, cfg->physics_dt);

	return PXL_SUCCESS;
}

/* Cleanup app and backend. */
static inline void
pxl_app_deinit(pxl_app_t *app) {
	assert(app);
	pxl_backend_deinit();
}

/* Internal: updates frame state */
static inline void
app_update_state(pxl_app_t *app, double frame_dt, double now) {
	app->prev = app->curr;
	app->curr.mouse_wheel_x = 0;
	app->curr.mouse_wheel_y = 0;
	app->frame_dt = frame_dt;
	app->prev_time = now;
	pxl_stepper_update(&app->physics_ts, pxl_app_dt(app));
}

/* Advance one frame using poll mode (non-blocking, active loop). */
static inline bool
pxl_app_advance(pxl_app_t *app) {
	assert(app);

	double now = pxl_backend_get_time();
	double frame_dt = now - app->prev_time;

	/* Clamp frame time to avoid accumulation */
	if (frame_dt > PXL_APP_MAX_FRAME_TIME) {
		frame_dt = PXL_APP_MAX_FRAME_TIME;
	}

	app_update_state(app, frame_dt, now);

	pxl_backend_poll_events(&app->curr);

	return !pxl_input_state(&app->curr, PXL_WM_QUIT);
}

/* Advance one frame using wait mode (blocking until event). */
static inline bool
pxl_app_advance_wait(pxl_app_t *app) {
	assert(app);

	double now = pxl_backend_get_time();
	double frame_dt = now - app->prev_time;

	/* Clamp frame time to avoid accumulation */
	if (frame_dt > PXL_APP_MAX_FRAME_TIME) {
		frame_dt = PXL_APP_MAX_FRAME_TIME;
	}

	app_update_state(app, frame_dt, now);

	pxl_backend_wait_events(&app->curr);

	return !pxl_input_state(&app->curr, PXL_WM_QUIT);
}

/* Advance physics stepper by one fixed step. */
static inline bool
pxl_app_advance_physics(pxl_app_t *app) {
	assert(app);
	return pxl_stepper_advance(&app->physics_ts);
}

/* --- Input Helpers --- */
static inline bool
pxl_app_is_active(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return pxl_input_state(&app->curr, code);
}

static inline bool
pxl_app_was_triggered(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return pxl_input_state(&app->curr, code) &&
	       !pxl_input_state(&app->prev, code);
}

static inline bool
pxl_app_was_released(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return !pxl_input_state(&app->curr, code) &&
	       pxl_input_state(&app->prev, code);
}

#endif /* PXL_APP_H */
