#ifndef PXL_CAMERA_H
#define PXL_CAMERA_H

#include <assert.h>
#include <math.h>

#include "geom.h"
#include "canvas.h"

/*
 * Camera system for 2D rendering with zoom and panning support.
 *
 * This provides a simple way to implement:
 *   - Zoom in/out (scaling)
 *   - Camera panning (translation)
 *   - Viewport management
 *   - Coordinate space transformation (world <-> screen)
 *
 * The camera maintains a transformation that can be applied to a canvas
 * to convert from world coordinates to screen coordinates.
 *
 * Usage:
 *   pxl_camera_t cam;
 *   pxl_camera_init(&cam, 800, 600);  // World size (logical)
 *   pxl_camera_set_zoom(&cam, 2.0f);   // Zoom 2x
 *   pxl_camera_set_pos(&cam, 100, 100); // Pan to (100, 100)
 *
 *   // In render loop:
 *   pxl_camera_apply(&cam, &cnv);      // Apply transformation to canvas
 *   pxl_draw_rect(&cnv, x, y, w, h, color);  // x,y in world coordinates
 *   pxl_camera_reset(&cam, &cnv);      // Reset canvas to identity
 */

typedef struct {
    float x, y;           /* Camera position in world coordinates (center point) */
    float zoom;           /* Zoom factor (1.0 = 100%, 2.0 = 200%, 0.5 = 50%) */
    int world_w, world_h; /* World dimensions (logical size) */
    int viewport_w, viewport_h; /* Viewport dimensions (usually = physical size) */
} pxl_camera_t;

/* Initialize camera with world dimensions */
static inline void
pxl_camera_init(pxl_camera_t *cam, int world_w, int world_h) {
    assert(cam);
    cam->x = 0.0f;
    cam->y = 0.0f;
    cam->zoom = 1.0f;
    cam->world_w = world_w;
    cam->world_h = world_h;
    cam->viewport_w = world_w;
    cam->viewport_h = world_h;
}

/* Set camera position (center point in world coordinates) */
static inline void
pxl_camera_set_pos(pxl_camera_t *cam, float x, float y) {
    assert(cam);
    cam->x = x;
    cam->y = y;
}

/* Move camera by delta */
static inline void
pxl_camera_move(pxl_camera_t *cam, float dx, float dy) {
    assert(cam);
    cam->x += dx;
    cam->y += dy;
}

/* Set zoom factor */
static inline void
pxl_camera_set_zoom(pxl_camera_t *cam, float zoom) {
    assert(cam);
    cam->zoom = zoom < 0.001f ? 0.001f : zoom;  /* Clamp to minimum zoom */
}

/* Get current zoom factor */
static inline float
pxl_camera_get_zoom(const pxl_camera_t *cam) {
    assert(cam);
    return cam->zoom;
}

/* Zoom by a relative factor (e.g., 1.1f for 10% zoom in) */
static inline void
pxl_camera_zoom(pxl_camera_t *cam, float factor) {
    assert(cam);
    cam->zoom *= factor;
    if (cam->zoom < 0.001f) cam->zoom = 0.001f;
}

/* Set viewport size (usually matches physical display size) */
static inline void
pxl_camera_set_viewport(pxl_camera_t *cam, int viewport_w, int viewport_h) {
    assert(cam);
    cam->viewport_w = viewport_w;
    cam->viewport_h = viewport_h;
}

/* Convert world coordinates to screen coordinates */
static inline void
pxl_camera_world_to_screen(const pxl_camera_t *cam, float world_x, float world_y,
                            float *out_screen_x, float *out_screen_y) {
    assert(cam && out_screen_x && out_screen_y);
    *out_screen_x = (world_x - cam->x) * cam->zoom + (float)cam->viewport_w / 2.0f;
    *out_screen_y = (world_y - cam->y) * cam->zoom + (float)cam->viewport_h / 2.0f;
}

/* Convert screen coordinates to world coordinates */
static inline void
pxl_camera_screen_to_world(const pxl_camera_t *cam, float screen_x, float screen_y,
                            float *out_world_x, float *out_world_y) {
    assert(cam && out_world_x && out_world_y);
    *out_world_x = (screen_x - (float)cam->viewport_w / 2.0f) / cam->zoom + cam->x;
    *out_world_y = (screen_y - (float)cam->viewport_h / 2.0f) / cam->zoom + cam->y;
}

/* Apply camera transformation to a canvas.
 * This sets the canvas offset to account for camera panning.
 * Note: For zoom, the application should manually scale coordinates when drawing,
 * or use the helper functions like pxl_camera_world_to_screen_rect().
 */
static inline void
pxl_camera_apply(const pxl_camera_t *cam, pxl_canvas_t *cnv) {
    assert(cam && cnv);
    /* Calculate offset to center the camera view in the viewport */
    int offset_x = (int)((cam->viewport_w / 2.0f) - (cam->x * cam->zoom));
    int offset_y = (int)((cam->viewport_h / 2.0f) - (cam->y * cam->zoom));
    pxl_canvas_set_offset(cnv, offset_x, offset_y);
}

/* Reset canvas to identity (remove camera transformation) */
static inline void
pxl_camera_reset(const pxl_camera_t *cam, pxl_canvas_t *cnv) {
    (void)cam;  /* Unused */
    assert(cnv);
    pxl_canvas_reset_offset(cnv);
}

/* Get camera bounds in world coordinates */
static inline void
pxl_camera_get_bounds(const pxl_camera_t *cam, pxl_rect_t *out_bounds) {
    assert(cam && out_bounds);
    float half_w = (float)cam->viewport_w / (2.0f * cam->zoom);
    float half_h = (float)cam->viewport_h / (2.0f * cam->zoom);
    out_bounds->x = (int)(cam->x - half_w);
    out_bounds->y = (int)(cam->y - half_h);
    out_bounds->w = (int)(cam->viewport_w / cam->zoom);
    out_bounds->h = (int)(cam->viewport_h / cam->zoom);
}

/* Clamp camera to world bounds (prevents seeing outside the world) */
static inline void
pxl_camera_clamp_to_world(pxl_camera_t *cam) {
    assert(cam);
    float half_w = (float)cam->viewport_w / (2.0f * cam->zoom);
    float half_h = (float)cam->viewport_h / (2.0f * cam->zoom);
    
    if (cam->x - half_w < 0) cam->x = half_w;
    if (cam->x + half_w > cam->world_w) cam->x = cam->world_w - half_w;
    if (cam->y - half_h < 0) cam->y = half_h;
    if (cam->y + half_h > cam->world_h) cam->y = cam->world_h - half_h;
}

/* Helper: Transform a rectangle from world to screen coordinates */
static inline pxl_rect_t
pxl_camera_world_to_screen_rect(const pxl_camera_t *cam, pxl_rect_t world_rect) {
    pxl_rect_t screen_rect;
    pxl_camera_world_to_screen(cam, (float)world_rect.x, (float)world_rect.y,
                               (float*)&screen_rect.x, (float*)&screen_rect.y);
    screen_rect.w = (int)((float)world_rect.w * cam->zoom);
    screen_rect.h = (int)((float)world_rect.h * cam->zoom);
    return screen_rect;
}

/* Helper: Transform a point from world to screen coordinates */
static inline pxl_point_t
pxl_camera_world_to_screen_point(const pxl_camera_t *cam, pxl_point_t world_point) {
    pxl_point_t screen_point;
    pxl_camera_world_to_screen(cam, (float)world_point.x, (float)world_point.y,
                               (float*)&screen_point.x, (float*)&screen_point.y);
    return screen_point;
}

#endif /* PXL_CAMERA_H */
