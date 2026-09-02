#ifndef PXL_APP_H
#define PXL_APP_H

#include <assert.h>
#include <stdbool.h>

#include "err.h"
#include "backend.h"
#include "input.h"
#include "stepper.h"

/*
 * PXL App Layer - Frame management and input transitions
 *
 * This header provides utilities for managing input state across frames,
 * including detection of pressed/released transitions.
 *
 * Use this if your application needs to detect:
 *   - when a key was pressed THIS frame (was up last frame, down now)
 *   - when a key was released THIS frame (was down last frame, up now)
 *
 * Two event modes are available:
 *   - pxl_app_advance(): Uses backend_poll_events (active loop, high CPU)
 *     Suitable for games/animations that need to run at full speed
 *   - pxl_app_advance_wait(): Uses backend_wait_events (passive loop, low CPU)
 *     Suitable for editors/viewers that only need to update on input
 *
 * Physics stepper is optional: set physics_dt to 0 to disable.
 */

typedef struct {
	/* User configurable attributes */
	const char* title;
	int width, height;
	pxl_backend_flags_t backend_flags;

	double physics_dt;              /* Fixed timestep in seconds. 0 = disable physics stepper */
	
	/* Internal managed */
	pxl_input_t curr;
	pxl_input_t prev;
	
	pxl_time_stepper_t physics_ts;  /* stepper state (disabled if physics_dt == 0) */
} pxl_app_t;

/* Initialize the app and backend */
static inline pxl_err_t
pxl_app_init(pxl_app_t *app) {
	assert(app);
	
	if (pxl_backend_init(app->title, app->width, app->height, app->backend_flags) != PXL_SUCCESS) {
		return PXL_E_BACKEND_INIT;
	}
	
	app->curr = (pxl_input_t){0};
	app->prev = (pxl_input_t){0};
	
	/* Initialize physics stepper (dt = 0 means disabled) */
	app->physics_ts.dt = app->physics_dt;
	pxl_stepper_init(&app->physics_ts, pxl_backend_get_time());
	
	return PXL_SUCCESS;
}

/* Cleanup the app and backend */
static inline void
pxl_app_deinit(pxl_app_t *app) {
	assert(app);
	pxl_backend_deinit();
}

/* Check if the app should close (WM_QUIT only; user handles other keys) */
static inline bool
pxl_app_should_close(const pxl_app_t *app) {
	assert(app);
	return pxl_input_state(&app->curr, PXL_WM_QUIT);
}

/* Advance one frame using poll mode (non-blocking, active loop) */
static inline bool
pxl_app_advance(pxl_app_t *app) {
	assert(app);
	
	pxl_stepper_sync_time(&app->physics_ts, pxl_backend_get_time());

	app->prev = app->curr;
	app->curr.mouse_wheel_x = 0;
	app->curr.mouse_wheel_y = 0;

	pxl_backend_poll_events(&app->curr);

	/* Update app width/height if window was resized */
	if (pxl_input_state(&app->curr, PXL_WM_RESIZE)) {
		app->width = app->curr.window_width;
		app->height = app->curr.window_height;
	}

	return !pxl_app_should_close(app);
}

/* Advance one frame using wait mode (blocking until event) */
static inline bool
pxl_app_advance_wait(pxl_app_t *app) {
	assert(app);
	
	pxl_stepper_sync_time(&app->physics_ts, pxl_backend_get_time());

	app->prev = app->curr;
	app->curr.mouse_wheel_x = 0;
	app->curr.mouse_wheel_y = 0;

	pxl_backend_wait_events(&app->curr);

	/* Update app width/height if window was resized */
	if (pxl_input_state(&app->curr, PXL_WM_RESIZE)) {
		app->width = app->curr.window_width;
		app->height = app->curr.window_height;
	}

	return !pxl_app_should_close(app);
}

/* Advance physics stepper by one fixed step.
 * Returns false if physics is disabled (physics_dt == 0) or if interpolating. */
static inline bool
pxl_app_advance_physics(pxl_app_t *app) {
	assert(app);
	return pxl_stepper_advance(&app->physics_ts);
}

/* Check if key is currently pressed */
static inline bool
pxl_app_is_pressed(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return pxl_input_state(&app->curr, code);
}

/* Check if key was pressed this frame (transition from up to down) */
static inline bool
pxl_app_was_pressed(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return pxl_input_state(&app->curr, code) &&
	       !pxl_input_state(&app->prev, code);
}

/* Check if key was released this frame (transition from down to up) */
static inline bool
pxl_app_was_released(const pxl_app_t *app, pxl_input_code_t code) {
	assert(app);
	return !pxl_input_state(&app->curr, code) &&
	       pxl_input_state(&app->prev, code);
}

#endif /* PXL_APP_H */
