#include <assert.h>
#include <limits.h>  /* for INT_MAX */

#include "blit.h"
#include "geom.h"  /* for pxl_rect_t */
#include "tileset.h"

/* Draw a tile from a tileset to canvas at (x, y).
 * Uses pxl_blit_rect internally. Respects canvas offset and scissor.
 */

void
pxl_draw_tile(pxl_canvas_t *cnv, const pxl_tileset_t *ts, int tile_idx, int x, int y) {
	assert(cnv && cnv->pb);
	assert(ts && ts->atlas && ts->atlas->data);
	assert(ts->tile_w > 0 && ts->tile_h > 0);
	assert(ts->cols > 0 && ts->rows > 0);
	/* Prevent integer overflow in tile coordinate calculations */
	assert(ts->tile_w <= INT_MAX / ts->cols);
	assert(ts->tile_h <= INT_MAX / ts->rows);
	assert(ts->tile_w * ts->cols <= ts->atlas->width);
	assert(ts->tile_h * ts->rows <= ts->atlas->height);
	assert(tile_idx >= 0 && tile_idx < ts->cols * ts->rows);

	// Map linear tile index to grid coordinates: column = idx % cols, row = idx / cols
	int col = tile_idx % ts->cols;
	int row = tile_idx / ts->cols;

	pxl_rect_t src_rect = {
		.x = col * ts->tile_w,
		.y = row * ts->tile_h,
		.w = ts->tile_w,
		.h = ts->tile_h
	};

	pxl_blit_rect(cnv, ts->atlas, src_rect, x, y);
}
