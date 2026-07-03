#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "pxl.h"

#define W 800
#define H 600
#define FPS 60.0f

#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000
#define RED   0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE  0xFF0000FF

// Loop types
typedef enum {
    LOOP_VARIABLE,
    LOOP_FIXED_ACCUM,
    LOOP_FIXED_LERP,
    LOOP_COUNT
} loop_type_t;

// Square state with its own loop management
typedef struct {
    float x, y;
    int w, h;
    uint32_t color;
    float speed;
    
    // Loop-specific state
    loop_type_t loop_type;
    
    // For fixed timestep modes
    pxl_time_stepper_t stepper;
    
    // For variable timestep
    double prev_time;
    
    // For interpolation
    float prev_x;
    
    // Target for movement
    float target_x;
} square_t;

// Demo state
typedef struct {
    square_t squares[3];
    float slowdown_factor;
    float start_x;
    float y_offsets[3];
} demo_state_t;

static void
init_square(square_t *s, float x, float y, uint32_t color, loop_type_t loop_type) {
    s->x = x;
    s->y = y;
    s->w = 40;
    s->h = 40;
    s->color = color;
    s->speed = 200.0f;  // pixels per second
    s->target_x = x;
    s->prev_x = x;
    s->loop_type = loop_type;
    
    // Initialize stepper for fixed timestep modes
    s->stepper.dt = 1.0f / FPS;
    pxl_stepper_init(&s->stepper, pxl_backend_get_time());
    s->prev_time = pxl_backend_get_time();
}

static void
update_square(square_t *s, float dt) {
    s->prev_x = s->x;
    
    // Oscillate between 0 and W - s->w
    s->target_x += s->speed * dt;
    if (s->target_x > W - s->w) {
        s->target_x = W - s->w;
        s->speed = -fabsf(s->speed);
    } else if (s->target_x < 0) {
        s->target_x = 0;
        s->speed = fabsf(s->speed);
    }
    s->x = s->target_x;
}

static void
square_update(square_t *s, float slowdown_factor) {
    switch (s->loop_type) {
        case LOOP_VARIABLE: {
            double now = pxl_backend_get_time();
            float dt = (float)(now - s->prev_time) * slowdown_factor;
            s->prev_time = now;
            update_square(s, dt);
            break;
        }
        case LOOP_FIXED_ACCUM: {
            pxl_stepper_sync_time(&s->stepper, pxl_backend_get_time());
            while (pxl_stepper_advance(&s->stepper)) {
                // For fixed timestep, apply slowdown by modifying effective dt
                update_square(s, (float)s->stepper.dt * slowdown_factor);
            }
            break;
        }
        case LOOP_FIXED_LERP: {
            // Save position before update for interpolation
            s->prev_x = s->x;
            pxl_stepper_sync_time(&s->stepper, pxl_backend_get_time());
            while (pxl_stepper_advance(&s->stepper)) {
                update_square(s, (float)s->stepper.dt * slowdown_factor);
            }
            break;
        }
        case LOOP_COUNT:
            break;
    }
}

static void
render_square(pxl_canvas_t *cnv, const square_t *s, float lerp_factor) {
    float draw_x = s->x;
    
    // Only interpolate for lerp mode
    if (s->loop_type == LOOP_FIXED_LERP) {
        draw_x = s->prev_x + (s->x - s->prev_x) * lerp_factor;
    }
    
    pxl_canvas_set_color(cnv, s->color);
    pxl_fill_rect(cnv, (int)draw_x, (int)s->y, s->w, s->h);
}

static void
log_fps(double now, float slowdown_factor) {
    static double t0 = 0;
    static int n = 0;
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        float fps = (float)n / (float)(now - t0);
        printf("FPS: %d | Slowdown: x%.0f | 1=normal 2/3/4=slower | R=reset\r", (int)fps, slowdown_factor);
        fflush(stdout);
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    printf("3 squares (Red=Variable, Green=Fixed Accum, Blue=Fixed Lerp)\n");
    printf("Keys: 1-4=slowdown factor, R=reset, ESC=quit\n\n");
    
    if (pxl_backend_init("PXL Loop Comparison", W, H, false) != PXL_SUCCESS)
        return 1;

    demo_state_t state;
    state.start_x = 100;
    state.y_offsets[0] = -60;
    state.y_offsets[1] = 0;
    state.y_offsets[2] = 60;
    state.slowdown_factor = 1.0f;
    
    // Initialize squares
    init_square(&state.squares[0], state.start_x, H / 2.0f + state.y_offsets[0], RED,   LOOP_VARIABLE);
    init_square(&state.squares[1], state.start_x, H / 2.0f + state.y_offsets[1], GREEN, LOOP_FIXED_ACCUM);
    init_square(&state.squares[2], state.start_x, H / 2.0f + state.y_offsets[2], BLUE,  LOOP_FIXED_LERP);

    pxl_input_state_t in;
    pxl_input_init_state(&in);

    while (!pxl_input_pressed(&in, PXL_KEYB_ESCAPE) && !pxl_input_pressed(&in, PXL_WM_QUIT)) {
        pxl_backend_poll_events(&in);
        
        // Reset on R key
        if (pxl_input_pressed(&in, PXL_KEYB_R)) {
            init_square(&state.squares[0], state.start_x, H / 2.0f + state.y_offsets[0], RED,   LOOP_VARIABLE);
            init_square(&state.squares[1], state.start_x, H / 2.0f + state.y_offsets[1], GREEN, LOOP_FIXED_ACCUM);
            init_square(&state.squares[2], state.start_x, H / 2.0f + state.y_offsets[2], BLUE,  LOOP_FIXED_LERP);
        }
        
        // FPS simulation controls
        if (pxl_input_pressed(&in, PXL_KEYB_1)) state.slowdown_factor = 1.0f;  // Normal (60 FPS)
        if (pxl_input_pressed(&in, PXL_KEYB_2)) state.slowdown_factor = 2.0f;  // Simulate 30 FPS
        if (pxl_input_pressed(&in, PXL_KEYB_3)) state.slowdown_factor = 3.0f;  // Simulate 20 FPS
        if (pxl_input_pressed(&in, PXL_KEYB_4)) state.slowdown_factor = 4.0f;  // Simulate 15 FPS
        
        // Update each square with its own loop type
        for (int i = 0; i < 3; i++) {
            square_update(&state.squares[i], state.slowdown_factor);
        }
        
        // Get lerp factor from blue square (Fixed Lerp) for rendering
        float lerp_factor = state.squares[2].stepper.lerp_factor;

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            // Clear
            pxl_canvas_set_color(&cnv, BLACK);
            pxl_canvas_clear(&cnv);
            
            // Draw all squares
            for (int i = 0; i < 3; i++) {
                render_square(&cnv, &state.squares[i], lerp_factor);
            }
            
            log_fps(pxl_backend_get_time(), state.slowdown_factor);
            pxl_backend_end_frame();
        }
    }

    printf("\n");  // Clean up FPS line
    pxl_backend_deinit();
    return 0;
}
