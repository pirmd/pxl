#ifndef CANVAS_H
#define CANVAS_H

#include <stdbool.h>
#include "pixbuf.h"
#include "geom.h"

typedef struct {
	pixbuf_t  *pb;	       /* Target (borrowed)  */
	rect_t	   scissor;    /* Clipping rectangle */
	pix_t	   color;      /* Current color      */
} canvas_t;

/* Initialization */
void canvas_init(canvas_t *cnv, pixbuf_t *pb);

/* State */
void canvas_set_color(canvas_t *cnv, pix_t color);
void canvas_set_scissor(canvas_t *cnv, int x, int y, int w, int h);
void canvas_reset_scissor(canvas_t *cnv);

/* Visibility */
static inline bool
canvas_quick_reject(const canvas_t *cnv, int x, int y, int w, int h) {
	rect_t sc = cnv->scissor;
	return x >= sc.x + sc.w || x + w <= sc.x ||
	       y >= sc.y + sc.h || y + h <= sc.y;
}

/* Drawing */
void canvas_clear(canvas_t *cnv);
void canvas_blit_rect(canvas_t *cnv, int x, int y,
		const pixbuf_t *src, int sx, int sy, int w, int h);

#endif /* CANVAS_H */
