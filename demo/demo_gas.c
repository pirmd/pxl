/*
 * PXL Demo: Gas Simulation
 *
 * This is a MINIMAL physics simulation demo showcasing PXL's core features:
 *   - Window and event loop (pxl_app_init/advance/deinit)
 *   - Fixed-timestep physics (stepper.h via pxl_app_advance_physics)
 *   - Canvas-based rendering
 *   - Input handling (is_pressed vs was_pressed)
 *   - Time-based interpolation for smooth rendering at any framerate
 *
 * Architecture:
 *   Simulation state is updated in fixed timesteps (update_gas),
 *   then interpolated for rendering (interpolate_gas).
 *
 * To use as a boilerplate:
 *   1. Copy the main() structure
 *   2. Replace gas_t with your simulation state
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
#define FPS 60.0f

/* Simulation */
#define MAX_PARTICLES 500
#define PARTICLE_RADIUS 4

/* UI */
#define PAUSE_ZOOM 4

typedef struct {
	const pxl_font_t *font;
	bool show_pause;
	bool show_help;
} ui_t;

/* Gas simulation */

typedef struct {
	float x, y;
	float vx, vy;
	uint32_t color;
} particle_t;

typedef struct {
	particle_t particles[MAX_PARTICLES];
	size_t count;
	float width, height;
} gas_t;

/* Particle initialization with demo_rng() */
static void
particle_init(particle_t *p, float width, float height) {
	p->x = demo_rng_float_range(PARTICLE_RADIUS, width - PARTICLE_RADIUS);
	p->y = demo_rng_float_range(PARTICLE_RADIUS, height - PARTICLE_RADIUS);
	
	p->vx = demo_rng_float_range(-300.0f, 300.0f);
	p->vy = demo_rng_float_range(-300.0f, 300.0f);
	
	p->color = 0xFF000000 | (demo_rng() % 0xFFFFFF);
}

/* Initialize gas simulation */
static void
gas_init(gas_t *gas, size_t initial_count) {
	assert(gas != NULL);
	gas->count = initial_count;
	gas->width = W;
	gas->height = H;
	
	/* Initialize all particles with random positions and velocities */
	for (size_t i = 0; i < gas->count; i++) {
		particle_init(&gas->particles[i], gas->width, gas->height);
	}
}

/* Handle collision between two particles - simple bounce response */
static void
gas_handle_collision(particle_t *p1, particle_t *p2) {
	float dx = p2->x - p1->x;
	float dy = p2->y - p1->y;
	float dist2 = dx * dx + dy * dy;
	float min_dist = 2.0f * PARTICLE_RADIUS;
	
	/* Skip if particles are not overlapping */
	if (dist2 >= min_dist * min_dist) return;
	
	/* Simple bounce: reflect velocities along the collision normal */
	float dist = sqrtf(dist2);
	if (dist > 0) {
		float nx = dx / dist;
		float ny = dy / dist;
		
		/* Exchange velocity components along collision normal */
		float dot1 = p1->vx * nx + p1->vy * ny;
		float dot2 = p2->vx * nx + p2->vy * ny;
		
		p1->vx += (dot2 - dot1) * nx;
		p1->vy += (dot2 - dot1) * ny;
		p2->vx += (dot1 - dot2) * nx;
		p2->vy += (dot1 - dot2) * ny;
		
		/* Separate particles to prevent sticking */
		float overlap = (min_dist - dist) / 2.0f;
		p1->x -= overlap * nx;
		p1->y -= overlap * ny;
		p2->x += overlap * nx;
		p2->y += overlap * ny;
	}
}

static void
update_gas(const gas_t *current, float dt, int add_particles, gas_t *next) {
	assert(next != NULL && current != NULL);
	*next = *current;

	/* Apply particle count changes */
	if (add_particles != 0) {
		size_t new_count = (size_t)next->count + (size_t)add_particles;
		if (new_count > MAX_PARTICLES) new_count = MAX_PARTICLES;
		
		/* Initialize new particles */
		for (size_t i = next->count; i < new_count; i++) {
			particle_init(&next->particles[i], next->width, next->height);
		}
		next->count = new_count;
	}

	/* Move all particles */
	for (size_t i = 0; i < next->count; i++) {
		particle_t *p = &next->particles[i];
		p->x += p->vx * dt;
		p->y += p->vy * dt;
	}

	/* Boundary collisions (with energy loss for realism) */
	for (size_t i = 0; i < next->count; i++) {
		particle_t *p = &next->particles[i];
		
		if (p->x - PARTICLE_RADIUS < 0) {
			p->x = PARTICLE_RADIUS;
			p->vx = -p->vx * 0.95f;
		} else if (p->x + PARTICLE_RADIUS > next->width) {
			p->x = next->width - PARTICLE_RADIUS;
			p->vx = -p->vx * 0.95f;
		}
		
		if (p->y - PARTICLE_RADIUS < 0) {
			p->y = PARTICLE_RADIUS;
			p->vy = -p->vy * 0.95f;
		} else if (p->y + PARTICLE_RADIUS > next->height) {
			p->y = next->height - PARTICLE_RADIUS;
			p->vy = -p->vy * 0.95f;
		}
	}
	
	/* Particle-to-particle collisions: simple O(n^2) check */
	for (size_t i = 0; i < next->count; i++) {
		for (size_t j = i + 1; j < next->count; j++) {
			gas_handle_collision(&next->particles[i], &next->particles[j]);
		}
	}
}

static void
interpolate_gas(const gas_t *prev, const gas_t *cur, float alpha, gas_t *out) {
	assert(out != NULL && prev != NULL && cur != NULL);
	out->count = cur->count;
	out->width = cur->width;
	out->height = cur->height;
	
	/* Interpolate existing particles, copy new ones directly */
	for (size_t i = 0; i < cur->count; i++) {
		if (i < prev->count) {
			/* Interpolate between previous and current state */
			out->particles[i].x = prev->particles[i].x + (cur->particles[i].x - prev->particles[i].x) * alpha;
			out->particles[i].y = prev->particles[i].y + (cur->particles[i].y - prev->particles[i].y) * alpha;
			out->particles[i].vx = cur->particles[i].vx;
			out->particles[i].vy = cur->particles[i].vy;
			out->particles[i].color = cur->particles[i].color;
		} else {
			/* New particle: copy from current state */
			out->particles[i] = cur->particles[i];
		}
	}
}

/* Render */
static void
render_particle_count(pxl_canvas_t *cnv, const gas_t *gas, const ui_t *ui) {
	assert(cnv != NULL && gas != NULL && ui != NULL);
	
	char count_str[16];
	snprintf(count_str, sizeof(count_str), "Particles: %zu", gas->count);
	
	pxl_rect_t bounds = demo_text_bounds_scaled(ui->font, count_str, 1);
	uint32_t fg = 0xFFFFFFFF;
	pxl_canvas_set_color(cnv, fg);
	
	/* Draw at top-left */
	int x = 10;
	int y = 10 + bounds.h;
	demo_draw_text_scaled(cnv, ui->font, count_str, 1, x, y);
}

static void
render_gas(pxl_canvas_t *cnv, const gas_t *gas, const ui_t *ui) {
	assert(cnv != NULL && gas != NULL && ui != NULL);
	
	/* Draw all particles */
	for (size_t i = 0; i < gas->count; i++) {
		const particle_t *p = &gas->particles[i];
		pxl_canvas_set_color(cnv, p->color);
		pxl_fill_circle(cnv, (int)p->x, (int)p->y, PARTICLE_RADIUS);
	}
}

static void
render_pause(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);
	
	const char pause_str[] = "PAUSE";
	pxl_rect_t bounds = demo_text_bounds_scaled(ui->font, pause_str, PAUSE_ZOOM);
	
	int border = bounds.h / 6;
	int pad = bounds.h / 2;
	int x = W/2 - bounds.w / 2;
	int y = H/2 - bounds.h / 2;
	
	uint32_t fg = 0xFFFFFFFF;
	uint32_t bg = 0xFF000000;
	
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
	demo_draw_text_scaled(cnv, ui->font, pause_str, PAUSE_ZOOM, x, y);
}

static void
render_help(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);
	
	const char *help_lines[] = {
		"CONTROLS:",
		"",
		"Up/Down: add/remove particles",
		"PageUp/PageDown: speed",
		"P: pause",
		"H: help",
		"ESC: quit"
	};
	int line_count = sizeof(help_lines) / sizeof(help_lines[0]);
	int scale = 2;
	
	/* Calculate total bounds */
	int max_width = 0;
	int total_height = 0;
	for (int i = 0; i < line_count; i++) {
		pxl_rect_t bounds = demo_text_bounds_scaled(ui->font, help_lines[i], scale);
		if (bounds.w > max_width) max_width = bounds.w;
		total_height += bounds.h;
	}
	
	int border = scale * 4;
	int pad = scale * 6;
	int x = (W - max_width) / 2 - pad - border;
	int y = (H - total_height) / 2 - pad - border;
	
	uint32_t fg = 0xFFFFFFFF;
	uint32_t bg = 0xFF000000;
	
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
		pxl_rect_t bounds = demo_text_bounds_scaled(ui->font, help_lines[i], scale);
		int line_x = x + border + pad + (max_width - bounds.w) / 2;
		demo_draw_text_scaled(cnv, ui->font, help_lines[i], scale, line_x, current_y);
		current_y += bounds.h;
	}
}

/* Input handling */
static void
handle_input(pxl_app_t *app, ui_t *ui, int *add_particles) {
	assert(app != NULL && ui != NULL && add_particles != NULL);
	*add_particles = 0;
	
	/* UP/DOWN: add/remove particles (5 at a time) */
	if (pxl_app_is_pressed(app, PXL_KEYB_UP)) {
		*add_particles = +5;
	}
	if (pxl_app_is_pressed(app, PXL_KEYB_DOWN)) {
		*add_particles = -5;
	}
	
	/* Speed controls - Page Up/Down (time scale affects physics speed only) */
	if (pxl_app_was_pressed(app, PXL_KEYB_PAGE_UP)) {
		app->physics_ts.time_scale = fminf(app->physics_ts.time_scale * 1.25f, 8.0f);
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_PAGE_DOWN)) {
		app->physics_ts.time_scale = fmaxf(app->physics_ts.time_scale / 1.25f, 0.125f);
	}
	
	/* Focus-based pause: auto-pause on focus loss */
	bool auto_paused = pxl_app_is_pressed(app, PXL_WM_FOCUS_LOST) ||
		pxl_app_is_pressed(app, PXL_WM_MOUSE_FOCUS_LOST);
	if (auto_paused) {
		ui->show_pause = true;
	}
	
	/* Manual unpause: movement keys clear pause */
	if (pxl_app_is_pressed(app, PXL_KEYB_UP) || pxl_app_is_pressed(app, PXL_KEYB_DOWN)) {
		ui->show_pause = false;
	}
	
	/* Manual pause toggle */
	if (pxl_app_was_pressed(app, PXL_KEYB_P)) {
		ui->show_pause = !ui->show_pause;
	}
	
	/* Help screen toggle */
	ui->show_help = pxl_app_is_pressed(app, PXL_KEYB_H);
	
	/* Update physics pause state (stops simulation when paused) */
	app->physics_ts.paused = ui->show_pause || ui->show_help;
}

int
main(void) {
	printf("Gas simulation.\n"
		"Up/Down: add/remove particles, PageUp/PageDown: speed,\n"
		"P=pause, H=help, ESC=quit\n");

	pxl_app_t app = {
		.title = "PXL Gas Demo",
		.width = W,
		.height = H,
		.physics_hz = FPS,
		.backend_flags = 0
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	/* Initialize RNG */
	demo_rng_seed(0);

	/* Initialize UI */
	ui_t ui = {
		.font = &font_9x15_latin
	};

	/* Initialize gas simulation */
	gas_t gas;
	gas_init(&gas, 300);  /* Start with moderate particle count */

	/* Two states:
	 * - gas: current state (updated by physics)
	 * - gas_prev: previous state (for interpolation)
	 * This avoids "stuttering" when frame rate != physics rate.
	 */
	gas_t gas_prev = gas;

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

		int add_particles = 0;
		handle_input(&app, &ui, &add_particles);

		while (pxl_app_advance_physics(&app)) {
			gas_prev = gas;
			update_gas(&gas_prev, (float)app.physics_ts.dt, add_particles, &gas);
		}

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {

			/* Setup canvas */
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			/* Clear */
			pxl_canvas_set_color(&cnv, 0xFF000000);
			pxl_canvas_clear(&cnv);

			/* Smooth rendering via interpolation:
			 * Blends gas_prev and gas using lerp_factor.
			 * At 60 FPS with 60 physics steps: lerp_factor = 0 (no blend).
			 * At 120 FPS with 60 physics steps: lerp_factor = 0.5 (midpoint).
			 * This decouples rendering from physics for smooth visuals. */
			gas_t gas_interpolated;
			interpolate_gas(&gas_prev, &gas, app.physics_ts.lerp_factor, &gas_interpolated);

			/* Draw simulation */
			render_gas(&cnv, &gas_interpolated, &ui);
			render_particle_count(&cnv, &gas, &ui);

			/* Draw FPS in bottom right corner */
			char fps_str[16];
			snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
			pxl_rect_t fps_bounds = demo_text_bounds_scaled(ui.font, fps_str, 1);
			uint32_t fg = 0xFFFFFFFF;
			pxl_canvas_set_color(&cnv, fg);
			demo_draw_text_scaled(&cnv, ui.font, fps_str, 1,
				W - fps_bounds.w - 10, H - fps_bounds.h - 10);

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

	printf("\n");
	pxl_app_deinit(&app);
	return 0;
}
