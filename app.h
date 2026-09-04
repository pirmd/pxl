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
 * Design:
 *   - time_scale: Global time scale (1.0 = normal, 0.5 = slow-mo, 2.0 = fast).
 *   - paused: Global pause flag.
 *   - effective_dt: Precomputed dt for the frame (clamped, scaled, paused-aware).
 *   - ALWAYS call pxl_app_advance() every frame, even when paused, to:
 *       1. Keep physics_ts.accumulator consistent (no jumps on unpause).
 *       2. Process input events.
 *
 * Usage:
 *   pxl_app_t app = {
 *       .title = "My Game",
 *       .width = 800,
 *       .height = 600,
 *       .physics_dt = 1.0 / 60.0,  // Fixed timestep for physics
 *   };
 *   pxl_app_init(&app);
 *
 *   while (pxl_app_advance(&app)) {
 *       if (pxl_app_was_pressed(&app, PXL_KEY_P)) app.paused = !app.paused;
 *       if (pxl_app_was_pressed(&app, PXL_KEY_LEFT_SHIFT)) app.time_scale = 0.5f;
 *
 *       // Use app.effective_dt for custom timers:
 *       pxl_timer_advance(&my_timer, app.effective_dt);
 *
 *       // Physics updates:
 *       while (pxl_app_advance_physics(&app)) update_physics();
 *       render();
 *   }
 *   pxl_app_deinit(&app);
 */

typedef struct {
	/* --- User Configuration --- */
	const char* title;            /* Window title. */
	int width, height;            /* Window dimensions. */
	pxl_backend_flags_t backend_flags;  /* Backend-specific flags. */

	double physics_dt;            /* Fixed timestep for physics (0 = disable). */

	/* Time control (external, managed by user) */
	float time_scale;            /* Global time scale (default: 1.0). */
	bool paused;                  /* Global pause flag (default: false). */

	/* --- Internal State --- */
	pxl_input_t curr;             /* Current input state. */
	pxl_input_t prev;             /* Previous input state. */

	pxl_time_stepper_t physics_ts; /* Physics stepper state. */

	/* Time tracking */
	double prev_time;             /* Time at previous frame (for dt calculation). */
	double frame_dt;              /* Raw frame delta time (clamped, unaffected by time_scale/paused). */
	double effective_dt;          /* Effective dt for current frame (clamped, scaled, paused-aware). */
} pxl_app_t;

/* Initialize the app and backend. */
static inline pxl_err_t
pxl_app_init(pxl_app_t *app) {
	assert(app);

	if (pxl_backend_init(app->title, app->width, app->height, app->backend_flags) != PXL_SUCCESS) {
		return PXL_E_BACKEND_INIT;
	}

	app->curr = (pxl_input_t){0};
	app->prev = (pxl_input_t){0};

	/* Initialize time control */
	app->time_scale = 1.0f;
	app->paused = false;
	app->prev_time = pxl_backend_get_time();
	app->effective_dt = 0.0;

	/* Initialize physics stepper (dt = 0 means disabled) */
	pxl_stepper_init(&app->physics_ts, app->physics_dt);

	return PXL_SUCCESS;
}

/* Cleanup the app and backend. */
static inline void
pxl_app_deinit(pxl_app_t *app) {
	assert(app);
	pxl_backend_deinit();
}

/* Check if the app should close (WM_QUIT only; user handles other keys). */
static inline bool
pxl_app_should_close(const pxl_app_t *app) {
	assert(app);
	return pxl_input_state(&app->curr, PXL_WM_QUIT);
}

/* Advance one frame using poll mode (non-blocking, active loop). */
static inline bool
pxl_app_advance(pxl_app_t *app) {
	assert(app);

	double now = pxl_backend_get_time();
	double frame_dt = now - app->prev_time;

	/* Clamp to avoid spiral of death on frame drops */
	if (frame_dt > PXL_APP_MAX_FRAME_TIME) {
		frame_dt = PXL_APP_MAX_FRAME_TIME;
	}

	app->effective_dt = app->paused ? 0.0 : (frame_dt * (double)app->time_scale);

	/* Always update stepper (even when paused) to avoid time jumps on unpause */
	pxl_stepper_update(&app->physics_ts, app->effective_dt);

	app->prev = app->curr;
	app->curr.mouse_wheel_x = 0;
	app->curr.mouse_wheel_y = 0;
	app->frame_dt = frame_dt;
	app->prev_time = now;

	pxl_backend_poll_events(&app->curr);

	return !pxl_app_should_close(app);
}

/* Advance one frame using wait mode (blocking until event). */
static inline bool
pxl_app_advance_wait(pxl_app_t *app) {
	assert(app);

	double now = pxl_backend_get_time();
	double frame_dt = now - app->prev_time;

	if (frame_dt > PXL_APP_MAX_FRAME_TIME) {
		frame_dt = PXL_APP_MAX_FRAME_TIME;
	}

	app->effective_dt = app->paused ? 0.0 : (frame_dt * (double)app->time_scale);

	pxl_stepper_update(&app->physics_ts, app->effective_dt);

	app->prev = app->curr;
	app->curr.mouse_wheel_x = 0;
	app->curr.mouse_wheel_y = 0;
	app->frame_dt = frame_dt;
	app->prev_time = now;

	pxl_backend_wait_events(&app->curr);

	return !pxl_app_should_close(app);
}

/* Advance physics stepper by one fixed step. */
static inline bool
pxl_app_advance_physics(pxl_app_t *app) {
	assert(app);
	return pxl_stepper_advance(&app->physics_ts);
}

/* --- Input Helpers --- */
static inline bool
pxl_app_is_pressed(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return pxl_input_state(&app->curr, code);
}

static inline bool
pxl_app_was_pressed(const pxl_app_t *app, pxl_input_code_t code) {
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
