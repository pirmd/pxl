#include <assert.h>
#include <string.h>

#include "canvas.h"

void
canvas_init(canvas_t *cnv, pixbuf_t *pb) {
	assert(cnv);
	assert(pb && pb->data);

	cnv->pb      = pb;
	cnv->scissor = pixbuf_bounds(cnv->pb);
	cnv->color   = 0xFFFFFFFF;
}

void
canvas_set_color(canvas_t *cnv, pix_t color) {
	assert(cnv);
	cnv->color = color;
}

void
canvas_set_scissor(canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv && cnv->pb);
	assert(w >= 0 && h >= 0);

	if (!clip_rect(
			(rect_t){x, y, w, h},
			pixbuf_bounds(cnv->pb),
			&cnv->scissor
		)) {
		cnv->scissor = (rect_t){0};
	}	
}

void
canvas_reset_scissor(canvas_t *cnv) {
	assert(cnv && cnv->pb);
	
	cnv->scissor = pixbuf_bounds(cnv->pb);
}

void
canvas_clear(canvas_t *cnv) {
	assert(cnv && cnv->pb);
	
	rect_t   sc  = cnv->scissor;
	pixbuf_t *pb = cnv->pb;
	
	/* Fast path: scissor covers entire buffer (implies sc.x==0 && sc.y==0 due to clipping) */
	if (sc.w == pb->width && sc.h == pb->height) {
		if (cnv->color == 0) {
			memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pix_t));
			return;
		}
		if (cnv->color == 0xFFFFFFFFU) {
			memset(pb->data, 0xFF, pb->height * pb->stride * sizeof(pix_t));
			return;
		}
	}

	pix_t pix = cnv->color;

	pix_t   *row = pixbuf_ptr(pb, sc.x, sc.y);
	int   stride = pb->stride;

	for (int dy = 0; dy < sc.h; ++dy) {
		for (int dx = 0; dx < sc.w; ++dx) {
			row[dx] = pix;
		}
		row += stride;
	}
}

void
canvas_fill_span(canvas_t *cnv, int x, int y, int w) {
	assert(cnv && cnv->pb);
	assert(w >= 0);

	rect_t sc  = cnv->scissor;
	
	if (y < sc.y || y >= sc.y + sc.h) {
		return;
	}
	
	span_t span;
	if (!clip_span(
			(span_t){x, w},
			(span_t){sc.x, sc.w},
			&span
		)) {
		return;
	}

	pix_t pix = cnv->color;

	pix_t *row = pixbuf_ptr(cnv->pb, span.x, y);
	for (int dx = 0; dx < span.w; ++dx) {
		row[dx] = pix;
	}
}

void
canvas_blit_rect(canvas_t *cnv, int x, int y,
		 const pixbuf_t *src, int sx, int sy, int w, int h) {
	assert(cnv && cnv->pb);
	assert(src);
	assert(sx >= 0 && sy >= 0);
	assert(w >= 0 && h >= 0 );

	/* 1. Clip destination to scissor */
	rect_t dst_r;
	if (!clip_rect((rect_t){x, y, w, h}, cnv->scissor, &dst_r)) {
		return;
	}

	/* 2. Calculate offset from clipping and apply to source */
	sx += dst_r.x - x;
	sy += dst_r.y - y;

	/* 3. Clip source to src bounds */
	rect_t src_r;
	if (!clip_rect((rect_t){sx, sy, dst_r.w, dst_r.h}, pixbuf_bounds(src), &src_r)) {
		return;
	}

	/* 4. Sync dimensions after src clipping.
	   Note: dst_r.{x,y} remain unchanged as sx,sy >= 0 ensures src_r.{x,y} == sx,sy */
	dst_r.w = src_r.w;
	dst_r.h = src_r.h;

	/* 5. Copy src to dst */
	const pix_t *src_row = pixbuf_ptr(src, src_r.x, src_r.y);
	size_t    src_stride = src->stride;

	pix_t       *dst_row = pixbuf_ptr(cnv->pb, dst_r.x, dst_r.y);
	size_t    dst_stride = cnv->pb->stride;

	for (int dy = 0; dy < src_r.h; ++dy) {
		memcpy(dst_row, src_row, src_r.w * sizeof(pix_t));
		src_row += src_stride;
		dst_row += dst_stride; 
	}
}
