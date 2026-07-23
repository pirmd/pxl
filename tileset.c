#include <assert.h>

#include "draw.h"
#include "tileset.h"

/* Draw a tile from a tileset to canvas at (x, y).
 * Uses pxl_blit_rect internally. Respects canvas offset and scissor. */

void
pxl_draw_tile(pxl_canvas_t *cnv, const pxl_tileset_t *ts, int tile_idx, int x, int y) {
	assert(cnv && cnv->pb);
	assert(ts && ts->atlas && ts->atlas->data);
	assert(ts->tile_w > 0 && ts->tile_h > 0 && ts->cols > 0);
	assert(tile_idx >= 0 && tile_idx < ts->cols * (ts->atlas->height / ts->tile_h));

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
