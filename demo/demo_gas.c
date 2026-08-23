#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pxl.h"

#define W 800
#define H 600
#define FPS 60.0f
#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000

#define MAX_PARTICLES 3000
#define PARTICLE_RADIUS 4
#define CELL_SIZE (2 * PARTICLE_RADIUS + 1)

/*
 * Model
 */

typedef struct {
    float x, y;
    float vx, vy;
    uint32_t color;
} particle_t;

typedef struct {
    particle_t particles[MAX_PARTICLES];
    size_t count;
    float width, height;

    /* Spatial grid for optimized collision detection */
    int grid_width, grid_height;
    int *cell_head;
    int *cell_next;
} gas_t;

/* Initialize a particle with random position and velocity */
static void
particle_init(particle_t *p, float width, float height) {
    p->x = (float)(arc4random() % (int)(width - 2 * PARTICLE_RADIUS)) + PARTICLE_RADIUS;
    p->y = (float)(arc4random() % (int)(height - 2 * PARTICLE_RADIUS)) + PARTICLE_RADIUS;
    
    p->vx = (float)((int)(arc4random() % 300) - 150);
    p->vy = (float)((int)(arc4random() % 300) - 150);
    
    p->color = 0xFF000000 | (arc4random() % 0xFFFFFF);
}

/* Get cell index for a particle */
static int
gas_particle_to_cell(const gas_t *gas, const particle_t *p) {
    int cx = (int)(p->x / CELL_SIZE);
    int cy = (int)(p->y / CELL_SIZE);
    cx = cx < 0 ? 0 : (cx >= gas->grid_width ? gas->grid_width - 1 : cx);
    cy = cy < 0 ? 0 : (cy >= gas->grid_height ? gas->grid_height - 1 : cy);
    return cy * gas->grid_width + cx;
}

/* Build spatial grid for collision detection */
static void
gas_build_grid(gas_t *gas) {
    int cell_count = gas->grid_width * gas->grid_height;
    
    for (int i = 0; i < cell_count; i++) {
        gas->cell_head[i] = -1;
    }
    
    for (size_t i = 0; i < gas->count; i++) {
        int cell_idx = gas_particle_to_cell(gas, &gas->particles[i]);
        gas->cell_next[i] = gas->cell_head[cell_idx];
        gas->cell_head[cell_idx] = (int)i;
    }
}

/* Initialize gas simulation */
static void
gas_init(gas_t *gas, size_t initial_count) {
    gas->count = initial_count;
    gas->width = W;
    gas->height = H;
    
    gas->grid_width = (int)ceil(W / CELL_SIZE);
    gas->grid_height = (int)ceil(H / CELL_SIZE);
    int cell_count = gas->grid_width * gas->grid_height;
    
    gas->cell_head = malloc(cell_count * sizeof(int));
    gas->cell_next = malloc(MAX_PARTICLES * sizeof(int));
    
    for (size_t i = 0; i < gas->count; i++) {
        particle_init(&gas->particles[i], gas->width, gas->height);
    }
    
    gas_build_grid(gas);
}

static void
gas_deinit(gas_t *gas) {
    free(gas->cell_head);
    free(gas->cell_next);
    gas->cell_head = NULL;
    gas->cell_next = NULL;
    gas->count = 0;
}

static void
gas_add_particles(gas_t *gas, size_t n) {
    size_t new_count = gas->count + n;
    if (new_count > MAX_PARTICLES) new_count = MAX_PARTICLES;
    
    for (size_t i = gas->count; i < new_count; i++) {
        particle_init(&gas->particles[i], gas->width, gas->height);
    }
    gas->count = new_count;
}

static void
gas_remove_particles(gas_t *gas, size_t n) {
    if (gas->count <= n) {
        gas->count = 0;
    } else {
        gas->count -= n;
    }
}

static void
gas_handle_collision(particle_t *p1, particle_t *p2) {
    float dx = p2->x - p1->x;
    float dy = p2->y - p1->y;
    float dist2 = dx * dx + dy * dy;
    float min_dist = 2.0f * PARTICLE_RADIUS;
    
    if (dist2 >= min_dist * min_dist) return;
    
    float dist = sqrtf(dist2);
    if (dist == 0) {
        dx = (float)((int)(arc4random() % 20) - 10);
        dy = (float)((int)(arc4random() % 20) - 10);
        dist = sqrtf(dx * dx + dy * dy);
        if (dist == 0) dist = 1.0f;
    }
    
    float nx = dx / dist;
    float ny = dy / dist;
    
    float rvx = p2->vx - p1->vx;
    float rvy = p2->vy - p1->vy;
    float dot = rvx * nx + rvy * ny;
    
    if (dot < 0) {
        p1->vx += dot * nx;
        p1->vy += dot * ny;
        p2->vx -= dot * nx;
        p2->vy -= dot * ny;
    }
    
    float overlap = (min_dist - dist) / 2.0f;
    p1->x -= overlap * nx;
    p1->y -= overlap * ny;
    p2->x += overlap * nx;
    p2->y += overlap * ny;
}

static void
update_gas(gas_t *gas, float dt) {
    /* Move all particles */
    for (size_t i = 0; i < gas->count; i++) {
        particle_t *p = &gas->particles[i];
        p->x += p->vx * dt;
        p->y += p->vy * dt;
    }
    
    /* Boundary collisions */
    for (size_t i = 0; i < gas->count; i++) {
        particle_t *p = &gas->particles[i];
        
        if (p->x - PARTICLE_RADIUS < 0) {
            p->x = PARTICLE_RADIUS;
            p->vx = -p->vx * 0.95f;
        } else if (p->x + PARTICLE_RADIUS > gas->width) {
            p->x = gas->width - PARTICLE_RADIUS;
            p->vx = -p->vx * 0.95f;
        }
        
        if (p->y - PARTICLE_RADIUS < 0) {
            p->y = PARTICLE_RADIUS;
            p->vy = -p->vy * 0.95f;
        } else if (p->y + PARTICLE_RADIUS > gas->height) {
            p->y = gas->height - PARTICLE_RADIUS;
            p->vy = -p->vy * 0.95f;
        }
    }
    
    /* Rebuild spatial grid */
    gas_build_grid(gas);
    
    /* Particle-to-particle collisions using spatial grid */
    for (size_t i = 0; i < gas->count; i++) {
        int cell_idx = gas_particle_to_cell(gas, &gas->particles[i]);
        int cx = cell_idx % gas->grid_width;
        int cy = cell_idx / gas->grid_width;
        
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = cx + dx;
                int ny = cy + dy;
                
                if (nx < 0 || nx >= gas->grid_width || ny < 0 || ny >= gas->grid_height) 
                    continue;
                
                int neighbor_cell = ny * gas->grid_width + nx;
                int j = gas->cell_head[neighbor_cell];
                
                while (j >= 0) {
                    if (j > (int)i) {
                        gas_handle_collision(&gas->particles[i], &gas->particles[j]);
                    }
                    j = gas->cell_next[j];
                }
            }
        }
    }
}

static void
interpolate_gas(gas_t *out, const gas_t *prev, const gas_t *cur, float alpha) {
    out->count = cur->count;
    out->width = cur->width;
    out->height = cur->height;
    
    for (size_t i = 0; i < cur->count; i++) {
        out->particles[i].x = prev->particles[i].x + (cur->particles[i].x - prev->particles[i].x) * alpha;
        out->particles[i].y = prev->particles[i].y + (cur->particles[i].y - prev->particles[i].y) * alpha;
        out->particles[i].vx = cur->particles[i].vx;
        out->particles[i].vy = cur->particles[i].vy;
        out->particles[i].color = cur->particles[i].color;
    }
}

static void
render_gas(pxl_canvas_t *cnv, const gas_t *gas) {
    pxl_canvas_set_color(cnv, BLACK);
    pxl_canvas_clear(cnv);
    
    /* Particle count indicator (1 white square = 100 particles) */
    pxl_canvas_set_color(cnv, WHITE);
    size_t indicator_count = gas->count / 100;
    if (indicator_count > 30) indicator_count = 30;
    for (size_t i = 0; i < indicator_count; i++) {
        pxl_fill_rect(cnv, W - 10 - (int)i * 10, 10, 8, 8);
    }
    
    /* Draw all particles */
    for (size_t i = 0; i < gas->count; i++) {
        const particle_t *p = &gas->particles[i];
        pxl_canvas_set_color(cnv, p->color);
        pxl_fill_circle(cnv, (int)p->x, (int)p->y, PARTICLE_RADIUS);
    }
}

/*
 * Application state
 */

struct {
	pxl_input_t in_prev;
	pxl_input_t in_curr;

	pxl_time_stepper_t stepper;
	bool paused;

	gas_t gas;
	gas_t gas_prev;

	float speed_factor;  /* 1.0 = normal, 2.0 = 2x, 4.0 = 4x, 0.5 = 0.5x */
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
log_fps(double now, size_t particle_count, float speed_factor) {
    static double t0 = 0;
    static int n = 0;
    
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        int current_fps = (int)((float)n / (float)(now - t0));
        printf("FPS: %d | Particles: %zu | Speed: x%.1f\r", current_fps, particle_count, speed_factor);
        fflush(stdout);
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    if (pxl_backend_init("PXL Gas Demo (Spatial Grid)", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Gas simulation. Up/Down: add/remove particles, 1-4: speed, P=pause, ESC=quit\n");

    gas_init(&app.gas, 200);
    app.gas_prev = app.gas;

    app.stepper.dt = 1.0f / FPS;
    pxl_stepper_init(&app.stepper, pxl_backend_get_time());
    app.speed_factor = 1.0f;

    while (!is_pressed(PXL_KEYB_ESCAPE) && !is_pressed(PXL_WM_QUIT)) {
        app.in_prev = app.in_curr;
        pxl_backend_poll_events(&app.in_curr);

        if (was_pressed(PXL_KEYB_P)) {
            app.paused = !app.paused;
        }

        /* Speed controls */
        if (was_pressed(PXL_KEYB_1)) app.speed_factor = 1.0f;
        if (was_pressed(PXL_KEYB_2)) app.speed_factor = 2.0f;
        if (was_pressed(PXL_KEYB_3)) app.speed_factor = 4.0f;
        if (was_pressed(PXL_KEYB_4)) app.speed_factor = 0.5f;

        pxl_stepper_sync_time(&app.stepper, pxl_backend_get_time());

        if (was_pressed(PXL_KEYB_K) || was_pressed(PXL_KEYB_UP)) {
            gas_add_particles(&app.gas, 10);
        }
        if (was_pressed(PXL_KEYB_J) || was_pressed(PXL_KEYB_DOWN)) {
            gas_remove_particles(&app.gas, 10);
        }

        if (!is_paused()) {
            while (pxl_stepper_advance(&app.stepper)) {
                app.gas_prev = app.gas;
                update_gas(&app.gas, (float)app.stepper.dt * app.speed_factor);
            }
        }

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            gas_t interpolated;
            interpolate_gas(&interpolated, &app.gas_prev, &app.gas, app.stepper.lerp_factor);
            render_gas(&cnv, &interpolated);
            
            if (is_paused()) {
                printf("PAUSED | Press P to resume\r");
                fflush(stdout);
            } else {
                log_fps(pxl_backend_get_time(), app.gas.count, app.speed_factor);
            }
            
            pxl_backend_end_frame();
        }
    }

    printf("\n");
    gas_deinit(&app.gas);
    pxl_backend_deinit();
    return 0;
}
