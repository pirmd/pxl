#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pxl.h"

#define W 800
#define H 600

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

/* Particle types */
typedef enum {
    PARTICLE_FIRE,
    PARTICLE_SMOKE,
    PARTICLE_SPARK,
    PARTICLE_COUNT
} particle_type_t;

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
    atlas->data = malloc(atlas->stride * atlas->height * sizeof(pxl_t));

    for (int row = 0; row < ATLAS_ROWS; row++) {
        for (int col = 0; col < ATLAS_COLS; col++) {
            int tile_idx = row * ATLAS_COLS + col;
            particle_type_t type = tile_idx / 4;
            int frame = tile_idx % 4;

            uint32_t color;
            switch (type % PARTICLE_COUNT) {
                case PARTICLE_FIRE:
                    color = RED | ((frame * 60) << 16);
                    break;
                case PARTICLE_SMOKE:
                    color = GRAY + (frame * 0x0F0F0F);
                    break;
                case PARTICLE_SPARK:
                    color = YELLOW | (frame * 0x111111);
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

    float angle = (float)(arc4random() % 360) * 0.0174532925f;
    float speed = 50.0f + (float)(arc4random() % 100);

    switch (type) {
        case PARTICLE_FIRE:
            p->vx = cosf(angle) * speed;
            p->vy = sinf(angle) * speed - 50.0f;
            p->max_life = 0.5f + (float)(arc4random() % 100) / 1000.0f;
            p->scale = 0.5f + (float)(arc4random() % 100) / 200.0f;
            break;
        case PARTICLE_SMOKE:
            p->vx = cosf(angle) * speed * 0.3f;
            p->vy = sinf(angle) * speed * 0.3f - 20.0f;
            p->max_life = 1.0f + (float)(arc4random() % 200) / 100.0f;
            p->scale = 0.3f + (float)(arc4random() % 100) / 100.0f;
            break;
        case PARTICLE_SPARK:
            p->vx = cosf(angle) * speed * 1.5f;
            p->vy = sinf(angle) * speed * 1.5f;
            p->max_life = 0.3f + (float)(arc4random() % 100) / 500.0f;
            p->scale = 0.2f + (float)(arc4random() % 100) / 300.0f;
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

static void
handle_input(fountain_t *fountain, pxl_input_t *in) {
    if (pxl_input_was_pressed(in, PXL_KEYB_1)) {
        fountain->emit_type = PARTICLE_FIRE;
    }
    if (pxl_input_was_pressed(in, PXL_KEYB_2)) {
        fountain->emit_type = PARTICLE_SMOKE;
    }
    if (pxl_input_was_pressed(in, PXL_KEYB_3)) {
        fountain->emit_type = PARTICLE_SPARK;
    }
    if (pxl_input_was_pressed(in, PXL_KEYB_UP) || pxl_input_was_pressed(in, PXL_KEYB_K)) {
        fountain->emit_rate += 5;
    }
    if (pxl_input_was_pressed(in, PXL_KEYB_DOWN) || pxl_input_was_pressed(in, PXL_KEYB_J)) {
        fountain->emit_rate = (fountain->emit_rate > 5) ? fountain->emit_rate - 5 : 5;
    }
    
    /* Change particle type with mouse buttons */
    if (pxl_input_was_pressed(in, PXL_MOUSE_LEFT)) {
        fountain->emit_type = (fountain->emit_type + 1) % PARTICLE_COUNT;
    }
    if (pxl_input_was_pressed(in, PXL_MOUSE_RIGHT)) {
        fountain->emit_type = (fountain->emit_type - 1 + PARTICLE_COUNT) % PARTICLE_COUNT;
    }
    
    /* Update source position to follow mouse cursor */
    if (in->cur.mouse_x >= 0 && in->cur.mouse_y >= 0) {
        fountain->source_x = (float)in->cur.mouse_x;
        fountain->source_y = (float)in->cur.mouse_y;
    }
}

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
        .cols = ATLAS_COLS
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

static int
count_active_particles(fountain_t *fountain) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (fountain->particles[i].active) count++;
    }
    return count;
}

static void
update_fountain(fountain_t *fountain, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_t *p = &fountain->particles[i];
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

    fountain->emit_accumulator += dt * fountain->emit_rate;
    if (fountain->emit_accumulator >= 1.0f) {
        fountain->emit_accumulator -= 1.0f;
        spawn_particles(fountain, fountain->source_x, fountain->source_y, fountain->emit_type, 5);
    }
}

static void
render_particle(pxl_canvas_t *cnv, fountain_t *fountain, const particle_t *p) {
    if (!p->active) return;

    int screen_x = (int)(p->x - PARTICLE_TILE_SIZE / 2 * p->scale);
    int screen_y = (int)(p->y - PARTICLE_TILE_SIZE / 2 * p->scale);

    pxl_draw_sprite(cnv, &fountain->tileset, &fountain->sprites[p->type],
                   p->current_frame, screen_x, screen_y);
}


static void
render_hud(pxl_canvas_t *cnv, fountain_t *fountain, int current_fps, pxl_input_state_t *in) {
    pxl_buf_t *pb = cnv->pb;
    int m_x = in->mouse_x, m_y = in->mouse_y;
    int active_particles = count_active_particles(fountain);

    char hud_str[64];

    if (m_x >= 0 && m_x < pb->width && m_y >= 0 && m_y < pb->height) {
        pxl_t color = *pxl_buf_ptr(pb, m_x, m_y);
        snprintf(hud_str, sizeof(hud_str), "FPS: %d | Particles: %d/%d | Mouse: %d,%d | Pixel: #%06X",
                 current_fps, active_particles, MAX_PARTICLES,
                 m_x, m_y, color & 0x00FFFFFF);
    } else {
        snprintf(hud_str, sizeof(hud_str), "FPS: %d | Particles: %d/%d | Mouse: n/a | Pixel: n/a",
                 current_fps, active_particles, MAX_PARTICLES);
    }

    pxl_canvas_set_color(cnv, WHITE);
    pxl_draw_str(cnv, 10, H - 15, hud_str);
}

static void
render(pxl_canvas_t *cnv, fountain_t *fountain) {
    pxl_canvas_set_color(cnv, BLACK);
    pxl_canvas_clear(cnv);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        render_particle(cnv, fountain, &fountain->particles[i]);
    }
}

static void
update_fps(double now, int *current_fps) {
    static double t0 = 0;
    static int n = 0;
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        *current_fps = (int)((float)n / (float)(now - t0));
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    printf("Fountain Demo\n");
    printf("Controls: 1/2/3=type, Left/Right click=cycle type, Up/Down=rate, CTRL=show HUD, ESC=quit\n");
    printf("Particle fountain follows mouse cursor\n\n");

    if (pxl_backend_init("PXL Fountain", W, H, false) != PXL_SUCCESS)
        return 1;

    fountain_t fountain;
    init_fountain(&fountain);

    pxl_input_t in;
    pxl_input_init(&in);

    int current_fps = 0;
    double prev_time = pxl_backend_get_time();

    while (!pxl_input_is_pressed(&in, PXL_KEYB_ESCAPE) && !pxl_input_is_pressed(&in, PXL_WM_QUIT)) {
        pxl_input_next_state(&in);
        pxl_backend_poll_events(&in.cur);

        handle_input(&fountain, &in);

        double now = pxl_backend_get_time();
        float dt = (float)(now - prev_time);
        prev_time = now;

        update_fountain(&fountain, dt);

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);

            render(&cnv, &fountain);

			/* Show HUD when CTRL is pressed */
			if (pxl_input_is_pressed(&in, PXL_KEYB_LCTRL) || pxl_input_is_pressed(&in, PXL_KEYB_RCTRL)) {
				render_hud(&cnv, &fountain, current_fps, &in.cur);
			}

            pxl_backend_end_frame();
        }
        
        update_fps(now, &current_fps);
    }

    free(fountain.atlas.data);
    fountain.atlas.data = NULL;
    pxl_backend_deinit();
    return 0;
}
