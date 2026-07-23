/* Tile rendering module: tileset (grid atlas) + sprite (frame indices). */

#ifndef PXL_TILESET_H
#define PXL_TILESET_H

#include "canvas.h"
#include "buf.h"

/* Tileset: grid of equally-sized tiles from a single image buffer.
 * Tiles are arranged in a grid: tile (i) at column (i % cols), row (i / cols).
 */
typedef struct {
	const pxl_buf_t *atlas;  /* Source image (read-only) */
	int tile_w;             /* Tile width in pixels */
	int tile_h;             /* Tile height in pixels */
	int cols;               /* Number of columns in grid */
} pxl_tileset_t;

/* Sprite: references a set of tiles (frames) in a tileset.
 * If `frames` is NULL: frames are sequential (base_idx, base_idx+1, ..., base_idx+frame_count-1).
 * If `frames` is not NULL: frames are read from the `frames` array.
 */
typedef struct {
	int base_idx;           /* First tile index (if frames==NULL) */
	int frame_count;        /* Total number of frames */
	const uint16_t *frames; /* Frame tile indices (NULL = sequential) */
} pxl_sprite_t;

/* Get tile index for a given frame.
 * Asserts if sprite is NULL or frame_idx is out of bounds.
 */
static inline int
pxl_sprite_get_tile(const pxl_sprite_t *sprite, int frame_idx) {
	assert(sprite && frame_idx >= 0 && frame_idx < sprite->frame_count);
	return sprite->frames ? sprite->frames[frame_idx] : sprite->base_idx + frame_idx;
}

/* Draw a single tile from tileset at (x, y) on canvas.
 * Respects canvas offset and scissor.
 */
void pxl_draw_tile(pxl_canvas_t *cnv, const pxl_tileset_t *ts, int tile_idx, int x, int y);

/* Draw a sprite frame at (x, y) on canvas.
 * Respects canvas offset and scissor.
 */
static inline void
pxl_draw_sprite(pxl_canvas_t *cnv, const pxl_tileset_t *ts, const pxl_sprite_t *sprite, int frame_idx, int x, int y) {
	pxl_draw_tile(cnv, ts, pxl_sprite_get_tile(sprite, frame_idx), x, y);
}

#endif /* PXL_TILESET_H */
