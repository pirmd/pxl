#ifndef PXL_GEOM_H
#define PXL_GEOM_H

#include <assert.h>
#include <stdbool.h>

typedef struct { int x, y, w, h; } pxl_rect_t;
typedef struct { int x, w; } pxl_span_t;

static inline int pxl_min(int a, int b) { return (a < b) ? a : b; }
static inline int pxl_max(int a, int b) { return (a < b) ? b : a; }

/* Clip rect r to bounds. Returns true if intersection is non-empty. */
static inline bool
pxl_clip_rect(pxl_rect_t in, pxl_rect_t bounds, pxl_rect_t *out) {
	assert(in.w >= 0 && in.h >= 0);
	assert(bounds.w >= 0 && bounds.h >= 0);
	assert(out);

	out->x = pxl_max(in.x, bounds.x);
	out->y = pxl_max(in.y, bounds.y);
	out->w = pxl_min(in.x + in.w, bounds.x + bounds.w) - out->x;
	out->h = pxl_min(in.y + in.h, bounds.y + bounds.h) - out->y;

	return out->w > 0 && out->h > 0;
}

/* Clip a span to bounds. Returns true if span is at least partially visible */
static inline bool
pxl_clip_span(pxl_span_t in, pxl_span_t bounds, pxl_span_t *out) {
	assert(in.w >= 0);
	assert(bounds.w >= 0);
	assert(out);

	out->x = pxl_max(in.x, bounds.x);
	out->w = pxl_min(in.x + in.w, bounds.x + bounds.w) - out->x;
	
	return out->w > 0;
}

/* Returns true if point (x,y) is inside rect r (inclusive left/top, exclusive right/bottom) */
static inline bool
pxl_in_rect(int x, int y, pxl_rect_t r) {
	return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

#endif /* PXL_GEOM_H */
