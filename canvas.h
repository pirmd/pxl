#ifndef PXL_CANVAS_H
#define PXL_CANVAS_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "buf.h"
#include "geom.h"

typedef struct {
	pxl_buf_t      *pb;           /* Target (borrowed)  */
	pxl_rect_t      scissor;      /* Clipping rectangle */
	pxl_t          color;        /* Current color      */
} pxl_canvas_t;

/* Visibility */
static inline bool
pxl_canvas_quick_reject(const pxl_canvas_t *cnv, int x, int y, int w, int h) {
	const pxl_rect_t sc = cnv->scissor;
	return x >= sc.x + sc.w || x + w <= sc.x ||
	       y >= sc.y + sc.h || y + h <= sc.y;
}

/* Initialization */
static inline void
pxl_canvas_init(pxl_canvas_t *cnv, pxl_buf_t *pb) {
	assert(cnv);
	assert(pb && pb->data);

	cnv->pb      = pb;
	cnv->scissor = (pxl_rect_t){0, 0, pb->width, pb->height};
	cnv->color   = 0xFFFFFFFF;
}

/* State */
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

/* Drawing */
static inline void
pxl_canvas_clear(pxl_canvas_t *cnv) {
	assert(cnv && cnv->pb);
	
	pxl_rect_t sc  = cnv->scissor;
	pxl_buf_t *pb = cnv->pb;
	
	/* Fast path: scissor covers entire buffer (implies sc.x==0 && sc.y==0 due to clipping) */
	if (sc.w == pb->width && sc.h == pb->height) {
		if (cnv->color == 0) {
			memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pxl_t));
			return;
		}
		if (cnv->color == 0xFFFFFFFFU) {
			memset(pb->data, 0xFF, pb->height * pb->stride * sizeof(pxl_t));
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
