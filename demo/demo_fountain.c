/*
 * PXL Demo: Particle Fountain
 *
 * This is a COMPLETE particle system demo showcasing PXL's core features:
 *   - Window and event loop (pxl_app_init/advance/deinit)
 *   - Fixed-timestep physics (stepper.h via pxl_app_advance_physics)
 *   - Canvas-based rendering with scissor regions
 *   - Input handling (is_pressed vs was_pressed)
 *   - Time-based interpolation for smooth rendering at any framerate
 *   - Sprite/atlas rendering
 *   - Mouse cursor tracking
 *
 * Architecture:
 *   Fountain state is updated in fixed timesteps (update_fountain),
 *   then interpolated for rendering (interpolate_fountain).
 *
 * To use as a boilerplate:
 *   1. Copy the main() structure
 *   2. Replace fountain_t with your simulation state
 *   3. Keep pxl_* calls and stepper pattern
 *
 * Note: demo_helpers.h contains demo-specific utilities (NOT part of PXL core).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pxl.h"
#include "demo_helpers.h"
#include "font_9x15.h"

#define W 800
#define H 600
#define FPS 60.0f

/* Colors */
#define BLACK       0xFF000000
#define WHITE       0xFFFFFFFF
#define RED         0xFFFF0000
#define YELLOW      0xFFFFFF00
#define GRAY        0xFF808080
#define TRANSPARENT 0x00000000

/* Particle system settings */
#define MAX_PARTICLES 500
#define PARTICLE_TILE_SIZE 8
#define ATLAS_COLS 8
#define ATLAS_ROWS 8
#define ATLAS_WIDTH  (ATLAS_COLS * PARTICLE_TILE_SIZE)
#define ATLAS_HEIGHT (ATLAS_ROWS * PARTICLE_TILE_SIZE)

#define GRAVITY 400.0f

/* UI */
#define HUD_ZOOM 2

/* Particle types */
typedef enum {
    PARTICLE_FIRE,
    PARTICLE_SMOKE,
    PARTICLE_SPARK,
    PARTICLE_COUNT
} particle_type_t;

/* UI state */
typedef struct {
    const pxl_font_t *font;
    bool show_pause;
    bool show_help;
} ui_t;

/* Single particle */
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    particle_type_t type;
    int current_frame;
    float frame_timer;
    float scale;
    bool active;
} particle_t;

/* Fountain state */
typedef struct {
    particle_t particles[MAX_PARTICLES];
    pxl_buf_t atlas;
    pxl_tileset_t tileset;
    pxl_sprite_t sprites[PARTICLE_COUNT];
    int emit_rate;
    float emit_accumulator;
    particle_type_t emit_type;
    float source_x;
    float source_y;
} fountain_t;

static void
init_atlas(pxl_buf_t *atlas) {
    atlas->width = ATLAS_WIDTH;
    atlas->height = ATLAS_HEIGHT;
    atlas->stride = pxl_calc_stride(ATLAS_WIDTH);
    atlas->data = malloc((size_t)atlas->stride * (size_t)atlas->height * sizeof(pxl_t));

    for (int row = 0; row < ATLAS_ROWS; row++) {
        for (int col = 0; col < ATLAS_COLS; col++) {
            int tile_idx = row * ATLAS_COLS + col;
            particle_type_t type = (particle_type_t)(tile_idx / 4);
            int frame = tile_idx % 4;

            uint32_t color;
            switch (type % PARTICLE_COUNT) {
                case PARTICLE_FIRE:
                    color = RED | (((unsigned int)frame * 60u) << 16);
                    break;
                case PARTICLE_SMOKE:
                    color = GRAY + (unsigned int)frame * 0x0F0F0Fu;
                    break;
                case PARTICLE_SPARK:
                    color = YELLOW | (unsigned int)frame * 0x111111u;
                    break;
                default:
                    color = WHITE;
                    break;
            }

            for (int ty = 0; ty < PARTICLE_TILE_SIZE; ty++) {
                for (int tx = 0; tx < PARTICLE_TILE_SIZE; tx++) {
                    int px = col * PARTICLE_TILE_SIZE + tx;
                    int py = row * PARTICLE_TILE_SIZE + ty;

                    int center_x = PARTICLE_TILE_SIZE / 2;
                    int center_y = PARTICLE_TILE_SIZE / 2;
                    int dx = tx - center_x;
                    int dy = ty - center_y;
                    int dist_sq = dx * dx + dy * dy;
                    int radius_sq = (PARTICLE_TILE_SIZE / 2) * (PARTICLE_TILE_SIZE / 2);

                    if (type % PARTICLE_COUNT == PARTICLE_SPARK) {
                        if (tx == center_x || ty == center_y ||
                            (tx + ty == PARTICLE_TILE_SIZE - 1) ||
                            (tx == ty)) {
                            atlas->data[py * atlas->width + px] = color;
                        } else {
                            atlas->data[py * atlas->width + px] = TRANSPARENT;
                        }
                    } else {
                        int noise = (frame * 3 + tx + ty * 2) % 3 - 1;
                        if (dist_sq <= radius_sq + noise * 2) {
                            atlas->data[py * atlas->width + px] = color;
                        } else {
                            atlas->data[py * atlas->width + px] = TRANSPARENT;
                        }
                    }
                }
            }
        }
    }
}

static void
init_particle(particle_t *p, float x, float y, particle_type_t type) {
    p->x = x;
    p->y = y;
    p->type = type;
    p->current_frame = 0;
    p->frame_timer = 0.0f;
    p->active = true;

    float angle = (float)(demo_rng() % 360) * 0.0174532925f;
    float speed = 50.0f + (float)(demo_rng() % 100);

    switch (type) {
        case PARTICLE_FIRE:
            p->vx = cosf(angle) * speed;
            p->vy = sinf(angle) * speed - 50.0f;
            p->max_life = 0.5f + (float)(demo_rng() % 100) / 1000.0f;
            p->scale = 0.5f + (float)(demo_rng() % 100) / 200.0f;
            break;
        case PARTICLE_SMOKE:
            p->vx = cosf(angle) * speed * 0.3f;
            p->vy = sinf(angle) * speed * 0.3f - 20.0f;
            p->max_life = 1.0f + (float)(demo_rng() % 200) / 100.0f;
            p->scale = 0.3f + (float)(demo_rng() % 100) / 100.0f;
            break;
        case PARTICLE_SPARK:
            p->vx = cosf(angle) * speed * 1.5f;
            p->vy = sinf(angle) * speed * 1.5f;
            p->max_life = 0.3f + (float)(demo_rng() % 100) / 500.0f;
            p->scale = 0.2f + (float)(demo_rng() % 100) / 300.0f;
            break;
        default:
            p->vx = 0;
            p->vy = 0;
            p->max_life = 1.0f;
            p->scale = 1.0f;
            break;
    }
    p->life = p->max_life;
}

static void
init_sprites(fountain_t *fountain) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        fountain->sprites[i] = (pxl_sprite_t){
            .base_idx = i * 4,
            .frame_count = 4
        };
    }
}

/* Fountain input */
typedef struct {
	int emit_rate_change;
	bool emit_type_changed;
	particle_type_t emit_type;
	float source_x;
	float source_y;
} fountain_input_t;

static void
init_fountain_particles(fountain_t *fountain) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        fountain->particles[i].active = false;
    }
}

static void
init_fountain(fountain_t *fountain) {
    init_atlas(&fountain->atlas);
    fountain->tileset = (pxl_tileset_t){
        .atlas = &fountain->atlas,
        .tile_w = PARTICLE_TILE_SIZE,
        .tile_h = PARTICLE_TILE_SIZE,
        .cols = ATLAS_COLS,
        .rows = ATLAS_ROWS
    };

    init_sprites(fountain);
    init_fountain_particles(fountain);

    fountain->emit_rate = 10;
    fountain->emit_accumulator = 0.0f;
    fountain->emit_type = PARTICLE_FIRE;
    fountain->source_x = W / 2.0f;
    fountain->source_y = H / 4.0f;
}

static particle_t *
find_inactive_particle(fountain_t *fountain) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!fountain->particles[i].active) {
            return &fountain->particles[i];
        }
    }
    return NULL;
}

static void
spawn_particles(fountain_t *fountain, float x, float y, particle_type_t type, int count) {
    for (int i = 0; i < count; i++) {
        particle_t *p = find_inactive_particle(fountain);
        if (p) {
            init_particle(p, x, y, type);
        }
    }
}

static void
interpolate_fountain(const fountain_t *prev, const fountain_t *cur, float alpha, fountain_t *out) {
	assert(out != NULL && prev != NULL && cur != NULL);
	
	out->tileset = cur->tileset;
	out->atlas = cur->atlas;
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		out->sprites[i] = cur->sprites[i];
	}
	out->emit_rate = cur->emit_rate;
	out->emit_accumulator = cur->emit_accumulator;
	out->emit_type = cur->emit_type;
	out->source_x = cur->source_x;
	out->source_y = cur->source_y;

	for (int i = 0; i < MAX_PARTICLES; i++) {
		const particle_t *p_prev = &prev->particles[i];
		const particle_t *p_cur = &cur->particles[i];
		particle_t *p_out = &out->particles[i];

		if (!p_cur->active) {
			p_out->active = false;
			continue;
		}

		p_out->active = true;
		p_out->x = p_prev->x + (p_cur->x - p_prev->x) * alpha;
		p_out->y = p_prev->y + (p_cur->y - p_prev->y) * alpha;
		p_out->vx = p_cur->vx;
		p_out->vy = p_cur->vy;
		p_out->life = p_cur->life;
		p_out->max_life = p_cur->max_life;
		p_out->type = p_cur->type;
		p_out->current_frame = p_cur->current_frame;
		p_out->frame_timer = p_cur->frame_timer;
		p_out->scale = p_cur->scale;
	}
}

static int
count_active_particles(const fountain_t *fountain) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (fountain->particles[i].active) count++;
    }
    return count;
}

static void
update_fountain(const fountain_t *current, float dt, fountain_input_t input, fountain_t *next) {
	assert(next != NULL && current != NULL);
	*next = *current;

	/* Apply input */
	next->emit_rate += input.emit_rate_change;
	if (next->emit_rate < 5) next->emit_rate = 5;
	
	if (input.emit_type_changed) {
		next->emit_type = input.emit_type;
	}
	next->source_x = input.source_x;
	next->source_y = input.source_y;

	for (int i = 0; i < MAX_PARTICLES; i++) {
		particle_t *p = &next->particles[i];
		if (!p->active) continue;

		p->x += p->vx * dt;
		p->y += p->vy * dt;
		p->vy += GRAVITY * dt;

		p->life -= dt;
		if (p->life <= 0.0f) {
			p->active = false;
			continue;
		}

		p->frame_timer += dt * 20.0f;
		if (p->frame_timer >= 1.0f) {
			p->frame_timer -= 1.0f;
			p->current_frame = (p->current_frame + 1) % 4;
		}

		if (p->x < -PARTICLE_TILE_SIZE || p->x > W + PARTICLE_TILE_SIZE ||
		    p->y < -PARTICLE_TILE_SIZE || p->y > H + PARTICLE_TILE_SIZE) {
			p->active = false;
		}
	}

	next->emit_accumulator += dt * (float)next->emit_rate;
	if (next->emit_accumulator >= 1.0f) {
		next->emit_accumulator -= 1.0f;
		spawn_particles(next, next->source_x, next->source_y, next->emit_type, 5);
	}
}

static void
render_particle(pxl_canvas_t *cnv, const fountain_t *fountain, const particle_t *p) {
    if (!p->active) return;

    int screen_x = (int)(p->x - PARTICLE_TILE_SIZE / 2 * p->scale);
    int screen_y = (int)(p->y - PARTICLE_TILE_SIZE / 2 * p->scale);

    pxl_draw_sprite(cnv, &fountain->tileset, &fountain->sprites[p->type],
                   p->current_frame, screen_x, screen_y);
}

static void
render_particle_count(pxl_canvas_t *cnv, const fountain_t *fountain, const pxl_font_t *font) {
	assert(cnv != NULL && fountain != NULL && font != NULL);
	
	int active_particles = count_active_particles(fountain);
	char count_str[32];
	snprintf(count_str, sizeof(count_str), "Particles: %d/%d", active_particles, MAX_PARTICLES);
	
	pxl_rect_t bounds = demo_text_bounds_scaled(font, count_str, 1);
	uint32_t fg = 0xFFFFFFFF;
	pxl_canvas_set_color(cnv, fg);
	
	/* Draw at top-left */
	int x = 10;
	int y = 10 + bounds.h;
	 demo_draw_text_scaled(cnv, font, count_str, 1, x, y);
}

static void
render(pxl_canvas_t *cnv, const fountain_t *fountain) {
	assert(cnv != NULL && fountain != NULL);
	
	pxl_canvas_set_color(cnv, BLACK);
	pxl_canvas_clear(cnv);

	for (int i = 0; i < MAX_PARTICLES; i++) {
		render_particle(cnv, fountain, &fountain->particles[i]);
	}
}


static void
handle_fountain_input(pxl_app_t *app, particle_type_t current_type, fountain_input_t *input) {
	assert(app != NULL && input != NULL);
	*input = (fountain_input_t){0};

	/* Particle type selection */
	if (pxl_app_was_pressed(app, PXL_KEYB_1)) {
		input->emit_type = PARTICLE_FIRE;
		input->emit_type_changed = true;
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_2)) {
		input->emit_type = PARTICLE_SMOKE;
		input->emit_type_changed = true;
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_3)) {
		input->emit_type = PARTICLE_SPARK;
		input->emit_type_changed = true;
	}

	/* Emission rate */
	if (pxl_app_was_pressed(app, PXL_KEYB_UP) || pxl_app_was_pressed(app, PXL_KEYB_K)) {
		input->emit_rate_change = +5;
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_DOWN) || pxl_app_was_pressed(app, PXL_KEYB_J)) {
		input->emit_rate_change = -5;
	}

	/* Change particle type with keyboard or left mouse button */
	if (pxl_app_was_pressed(app, PXL_KEYB_LEFT)) {
		input->emit_type = (current_type - 1 + PARTICLE_COUNT) % PARTICLE_COUNT;
		input->emit_type_changed = true;
	}
	if (pxl_app_was_pressed(app, PXL_KEYB_RIGHT)) {
		input->emit_type = (current_type + 1) % PARTICLE_COUNT;
		input->emit_type_changed = true;
	}
	if (pxl_app_was_pressed(app, PXL_MOUSE_LEFT)) {
		input->emit_type = (current_type + 1) % PARTICLE_COUNT;
		input->emit_type_changed = true;
	}

	/* Update source position to follow mouse cursor */
	if (app->curr.mouse_x >= 0 && app->curr.mouse_y >= 0) {
		input->source_x = (float)app->curr.mouse_x;
		input->source_y = (float)app->curr.mouse_y;
	}
}

/* Render pause overlay */
static void
render_pause(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);

	const char pause_str[] = "PAUSE";
	pxl_rect_t bounds = demo_text_bounds_scaled(ui->font, pause_str, HUD_ZOOM);

	int border = bounds.h / 6;
	int pad = bounds.h / 2;
	int x = W/2 - bounds.w / 2;
	int y = H/2 - bounds.h / 2;

	uint32_t fg = WHITE;
	uint32_t bg = BLACK;

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
	 demo_draw_text_scaled(cnv, ui->font, pause_str, HUD_ZOOM, x, y);
}

/* Render help overlay */
static void
render_help(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv != NULL && ui != NULL);

	const char *help_lines[] = {
		"CONTROLS:",
		"",
		"1/2/3: particle type",
		"Left/Right: prev/next type",
		"Left click: next type",
		"Up/Down: emit rate",
		"Mouse: move fountain",
		"",
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

	uint32_t fg = WHITE;
	uint32_t bg = BLACK;

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

static void
handle_input(pxl_app_t *app, ui_t *ui, particle_type_t current_type, fountain_input_t *fountain_input) {
	assert(app != NULL && ui != NULL && fountain_input != NULL);

	/* Focus-based pause: auto-pause on focus loss */
	bool auto_paused = pxl_app_is_pressed(app, PXL_WM_FOCUS_LOST) ||
		pxl_app_is_pressed(app, PXL_WM_MOUSE_FOCUS_LOST);
	if (auto_paused) {
		ui->show_pause = true;
	}

	/* Manual unpause: mouse buttons clear pause */
	if (pxl_app_was_pressed(app, PXL_MOUSE_LEFT) || pxl_app_was_pressed(app, PXL_MOUSE_RIGHT)) {
		ui->show_pause = false;
	}

	/* Manual pause toggle */
	if (pxl_app_was_pressed(app, PXL_KEYB_P)) {
		ui->show_pause = !ui->show_pause;
	}

	/* Help screen toggle */
	ui->show_help = pxl_app_is_pressed(app, PXL_KEYB_H);

	/* Update physics pause state */
	app->physics_ts.paused = ui->show_pause || ui->show_help;

	/* Handle fountain-specific input if not paused */
	if (!ui->show_pause && !ui->show_help) {
		handle_fountain_input(app, current_type, fountain_input);
	}
}

int
main(void) {
	printf("Fountain Demo.\n"
	       "1/2/3=select type, Left/Right=prev/next type, Left click=next.\n"
	       "Up/Down=emit rate, Mouse=move fountain.\n"
	       "P=pause, H=help, ESC=quit\n\n");

	pxl_app_t app = {
		.title = "PXL Fountain",
		.width = W,
		.height = H,
		.physics_dt = 1.0 / FPS
	};

	if (pxl_app_init(&app) != PXL_SUCCESS)
		return 1;

	/* Initialize RNG */
	demo_rng_seed(0);

	/* Initialize fountain */
	fountain_t fountain;
	init_fountain(&fountain);
	fountain_t fountain_prev = fountain;

	/* Initialize UI */
	ui_t ui = {
		.font = &font_9x15_latin
	};

	int fps = 0;

	while (pxl_app_advance(&app)) {
		if (pxl_app_was_pressed(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		fountain_input_t fountain_input;
		handle_input(&app, &ui, fountain.emit_type, &fountain_input);

		while (pxl_app_advance_physics(&app)) {
			fountain_prev = fountain;
			update_fountain(&fountain_prev, (float)app.physics_ts.dt, fountain_input, &fountain);
		}

		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			/* Setup canvas */
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);

			/* Main canvas (full screen) */
			pxl_canvas_t cnv_main = cnv;
			pxl_canvas_set_scissor(&cnv_main, 0, 0, W, H);

			/* Interpolate and render */
			fountain_t fountain_interpolated;
			interpolate_fountain(&fountain_prev, &fountain, app.physics_ts.lerp_factor, &fountain_interpolated);
			render(&cnv_main, &fountain_interpolated);

			/* Render particle count at top-left */
			render_particle_count(&cnv_main, &fountain_interpolated, ui.font);

			/* Draw FPS in bottom right corner */
			if (fps > 0) {
				char fps_str[16];
				snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
				pxl_rect_t fps_bounds = demo_text_bounds_scaled(ui.font, fps_str, 1);
				uint32_t fg = 0xFFFFFFFF;
				pxl_canvas_set_color(&cnv_main, fg);
				demo_draw_text_scaled(&cnv_main, ui.font, fps_str, 1,
					W - fps_bounds.w - 10, H - fps_bounds.h - 10);
			}

			/* Draw pause overlay */
			if (ui.show_pause) {
				render_pause(&cnv_main, &ui);
			}

			/* Draw help overlay */
			if (ui.show_help) {
				render_help(&cnv_main, &ui);
			}

			(void)pxl_backend_end_frame();
		}
		
		demo_update_fps(pxl_backend_get_time(), &fps);
	}

	free(fountain.atlas.data);
	fountain.atlas.data = NULL;
	pxl_app_deinit(&app);
	return 0;
}
