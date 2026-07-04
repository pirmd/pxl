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

// Particle definition
typedef struct {
    float x, y;
    float vx, vy;
    uint32_t color;
} particle_t;

// Gas simulation state
typedef struct {
    particle_t particles[MAX_PARTICLES];
    size_t count;
    float width, height;
    // Spatial grid for optimized collision detection
    int grid_width, grid_height;
    int *cell_head;
    int *cell_next;
} gas_t;

// Initialize a particle with random position and velocity
static void
particle_init(particle_t *p, float width, float height) {
    p->x = (float)(arc4random() % (int)(width - 2 * PARTICLE_RADIUS)) + PARTICLE_RADIUS;
    p->y = (float)(arc4random() % (int)(height - 2 * PARTICLE_RADIUS)) + PARTICLE_RADIUS;
    
    p->vx = (float)((int)(arc4random() % 300) - 150);
    p->vy = (float)((int)(arc4random() % 300) - 150);
    
    p->color = 0xFF000000 | (arc4random() % 0xFFFFFF);
}

// Get cell index for a particle
static int
gas_particle_to_cell(const gas_t *gas, const particle_t *p) {
    int cx = (int)(p->x / CELL_SIZE);
    int cy = (int)(p->y / CELL_SIZE);
    cx = cx < 0 ? 0 : (cx >= gas->grid_width ? gas->grid_width - 1 : cx);
    cy = cy < 0 ? 0 : (cy >= gas->grid_height ? gas->grid_height - 1 : cy);
    return cy * gas->grid_width + cx;
}

// Build spatial grid for collision detection
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

// Initialize gas simulation
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

// Free gas simulation
static void
gas_free(gas_t *gas) {
    free(gas->cell_head);
    free(gas->cell_next);
    gas->cell_head = NULL;
    gas->cell_next = NULL;
    gas->count = 0;
}

// Add particles to the simulation
static void
gas_add_particles(gas_t *gas, size_t n) {
    size_t new_count = gas->count + n;
    if (new_count > MAX_PARTICLES) new_count = MAX_PARTICLES;
    
    for (size_t i = gas->count; i < new_count; i++) {
        particle_init(&gas->particles[i], gas->width, gas->height);
    }
    gas->count = new_count;
}

// Remove particles from the simulation
static void
gas_remove_particles(gas_t *gas, size_t n) {
    if (gas->count <= n) {
        gas->count = 0;
    } else {
        gas->count -= n;
    }
}

// Handle collision between two particles
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

// Update particle positions with boundary and particle collisions
static void
gas_update(gas_t *gas, float dt) {
    // Move all particles
    for (size_t i = 0; i < gas->count; i++) {
        particle_t *p = &gas->particles[i];
        p->x += p->vx * dt;
        p->y += p->vy * dt;
    }
    
    // Boundary collisions
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
    
    // Rebuild spatial grid
    gas_build_grid(gas);
    
    // Particle-to-particle collisions using spatial grid
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

// Interpolate between previous and current gas state
static void
gas_interpolate(gas_t *out, const gas_t *prev, const gas_t *cur, float alpha) {
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

// Render gas simulation
static void
gas_render(pxl_canvas_t *cnv, const gas_t *gas) {
    pxl_canvas_set_color(cnv, BLACK);
    pxl_canvas_clear(cnv);
    
    // Particle count indicator (1 white square = 100 particles)
    pxl_canvas_set_color(cnv, WHITE);
    size_t indicator_count = gas->count / 100;
    if (indicator_count > 30) indicator_count = 30;
    for (size_t i = 0; i < indicator_count; i++) {
        pxl_fill_rect(cnv, W - 10 - (int)i * 10, 10, 8, 8);
    }
    
    // Draw all particles
    for (size_t i = 0; i < gas->count; i++) {
        const particle_t *p = &gas->particles[i];
        pxl_canvas_set_color(cnv, p->color);
        pxl_fill_circle(cnv, (int)p->x, (int)p->y, PARTICLE_RADIUS);
    }
}

// Log FPS to stdout
static void
log_fps(double now, size_t particle_count) {
    static double t0 = 0;
    static int n = 0;
    
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        int current_fps = (int)((float)n / (float)(now - t0));
        printf("FPS: %d | Particles: %zu\r", current_fps, particle_count);
        fflush(stdout);
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    if (pxl_backend_init("PXL Gas Demo (Spatial Grid)", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Gas simulation with spatial grid. Up/Down arrow: add/remove 10 particles, ESC to quit\n");

    gas_t gas, gas_prev;
    gas_init(&gas, 200);
    gas_prev = gas;

    pxl_time_stepper_t ts;
    ts.dt = 1.0f / FPS;
    pxl_stepper_init(&ts, pxl_backend_get_time());

    pxl_input_state_t in;
    pxl_input_init_state(&in);

    while (!pxl_input_pressed(&in, PXL_KEYB_ESCAPE) && !pxl_input_pressed(&in, PXL_WM_QUIT)) {
        pxl_stepper_sync_time(&ts, pxl_backend_get_time());
        pxl_backend_poll_events(&in);

        if (pxl_input_pressed(&in, PXL_KEYB_UP)) {
            gas_add_particles(&gas, 10);
        }
        if (pxl_input_pressed(&in, PXL_KEYB_DOWN)) {
            gas_remove_particles(&gas, 10);
        }

        while (pxl_stepper_advance(&ts)) {
            gas_prev = gas;
            gas_update(&gas, (float)ts.dt);
        }

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            gas_t interpolated;
            gas_interpolate(&interpolated, &gas_prev, &gas, ts.lerp_factor);
            gas_render(&cnv, &interpolated);
            
            log_fps(pxl_backend_get_time(), gas.count);
            pxl_backend_end_frame();
        }
    }

    printf("\n");
    gas_free(&gas);
    pxl_backend_deinit();
    return 0;
}
