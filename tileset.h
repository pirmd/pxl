/* Tile rendering module: tileset (grid atlas) + sprite (frame indices). */

#ifndef PXL_TILESET_H
#define PXL_TILESET_H

#include "canvas.h"
#include "buf.h"

/* Tileset: grid of equally-sized tiles from a single image buffer.
 * Tiles are arranged in a grid: tile (i) at column (i % cols), row (i / cols).
 * Caller must ensure: tile_w * cols <= atlas->width, tile_h * rows <= atlas->height.
 */
typedef struct {
	const pxl_buf_t *atlas;  /* Source image (read-only) */
	int tile_w;             /* Tile width in pixels */
	int tile_h;             /* Tile height in pixels */
	int cols;               /* Number of columns in grid */
	int rows;               /* Number of rows in grid */
} pxl_tileset_t;

/* Sprite: references a set of tiles (frames) in a tileset.
 * If `frames` is NULL: frames are sequential (base_idx, base_idx+1, ..., base_idx+frame_count-1).
 * If `frames` is not NULL: frames are read from the `frames` array (base_idx is ignored).
 * Caller must ensure: frame_count > 0.
 */
typedef struct {
	int base_idx;           /* First tile index (used only if frames==NULL) */
	int frame_count;        /* Total number of frames (must be > 0) */
	const uint16_t *frames; /* Frame tile indices (NULL = sequential) */
} pxl_sprite_t;

/* Draw a single tile from tileset at (x, y) on canvas.
 * Respects canvas offset and scissor.
 */
void pxl_draw_tile(pxl_canvas_t *cnv, const pxl_tileset_t *ts, int tile_idx, int x, int y);

/* Draw a sprite frame at (x, y) on canvas.
 * Respects canvas offset and scissor.
 * If sprite->frames is NULL, uses sequential tiles starting from sprite->base_idx.
 * Otherwise, uses sprite->frames[frame_idx].
 */
static inline void
pxl_draw_sprite(pxl_canvas_t *cnv, const pxl_tileset_t *ts, const pxl_sprite_t *sprite, int frame_idx, int x, int y) {
	assert(sprite && sprite->frame_count > 0);
	assert(frame_idx >= 0 && frame_idx < sprite->frame_count);
	int tile_idx = sprite->frames ? sprite->frames[frame_idx] : sprite->base_idx + frame_idx;
	pxl_draw_tile(cnv, ts, tile_idx, x, y);
}

#endif /* PXL_TILESET_H */
