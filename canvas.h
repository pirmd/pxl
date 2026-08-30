#ifndef PXL_CANVAS_H
#define PXL_CANVAS_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "buf.h"
#include "geom.h"

/*
 * Canvas: Drawing context wrapping a pixel buffer.
 * Stores offset, scissor clipping area, and drawing color as state.
 * Users must apply offset and scissor manually in their drawing code.
 */

typedef struct {
	pxl_buf_t      *pb;                 /* Target                */
	int             offset_x, offset_y; /* Translation offset     */
	pxl_rect_t      scissor;            /* Drawing clipping area */
	pxl_t           color;              /* Drawing color         */
	
} pxl_canvas_t;

/* Initialization ---------------------------------------------------------- */
static inline void
pxl_canvas_init(pxl_canvas_t *cnv, pxl_buf_t *pb) {
	assert(cnv);
	assert(pb && pb->data);

	cnv->pb       = pb;
	cnv->offset_x = 0;
	cnv->offset_y = 0;
	cnv->scissor  = (pxl_rect_t){0, 0, pb->width, pb->height};
	cnv->color    = 0xFFFFFFFF;
}

/* State ------------------------------------------------------------------- */
static inline void
pxl_canvas_set_color(pxl_canvas_t *cnv, pxl_t color) {
	assert(cnv);
	cnv->color = color;
}

static inline void
pxl_canvas_set_scissor(pxl_canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv && cnv->pb);
	assert(w >= 0 && h >= 0);

	if (!pxl_clip_rect(
			(pxl_rect_t){x, y, w, h},
			(pxl_rect_t){0, 0, cnv->pb->width, cnv->pb->height},
			&cnv->scissor
		)) {
		cnv->scissor = (pxl_rect_t){0};
	}	
}

static inline void
pxl_canvas_reset_scissor(pxl_canvas_t *cnv) {
	assert(cnv && cnv->pb);
	
	cnv->scissor = (pxl_rect_t){0, 0, cnv->pb->width, cnv->pb->height};
}

static inline void
pxl_canvas_set_offset(pxl_canvas_t *cnv, int offset_x, int offset_y) {
	cnv->offset_x = offset_x;
	cnv->offset_y = offset_y;
}

static inline void
pxl_canvas_reset_offset(pxl_canvas_t *cnv) {
	cnv->offset_x = 0;
	cnv->offset_y = 0;
}

/* Drawing ----------------------------------------------------------------- */
static inline void
pxl_canvas_clear(pxl_canvas_t *cnv) {
	assert(cnv && cnv->pb);
	
	pxl_rect_t sc = cnv->scissor;
	pxl_buf_t *pb = cnv->pb;
	
	/* Fast path: scissor covers entire buffer (implies sc.x==0 && sc.y==0 due to clipping) */
	if (sc.w == pb->width && sc.h == pb->height) {
		/* Prevent integer overflow in memset size calculation */
		assert(pb->stride <= INT_MAX / (int)sizeof(pxl_t));
		assert(pb->height <= INT_MAX / pb->stride / (int)sizeof(pxl_t));

		if (cnv->color == 0) {
			memset(pb->data, 0x00, (size_t)pb->height * (size_t)pb->stride * sizeof(pxl_t));
			return;
		}
		if (cnv->color == 0xFFFFFFFFU) {
			memset(pb->data, 0xFF, (size_t)pb->height * (size_t)pb->stride * sizeof(pxl_t));
			return;
		}
	}

	pxl_t pix = cnv->color;

	pxl_t   *row = pxl_buf_ptr(pb, sc.x, sc.y);
	int   stride = pb->stride;

	for (int dy = 0; dy < sc.h; ++dy) {
		for (int dx = 0; dx < sc.w; ++dx) {
			row[dx] = pix;
		}
		row += stride;
	}
}

#endif /* PXL_CANVAS_H */	
