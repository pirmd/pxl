#ifndef PXL_GEOM_H
#define PXL_GEOM_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>

typedef struct { int x, y, w, h; } pxl_rect_t;
typedef struct { int x, w; } pxl_span_t;

/* Alignment flags (bitmask). Combine with | for horizontal and vertical.
 * Example: PXL_H_CENTER | PXL_V_CENTER */
typedef enum {
	/* Horizontal alignment (bits 0-1) */
	PXL_H_LEFT   = 0,
	PXL_H_CENTER = 1,
	PXL_H_RIGHT  = 2,

	/* Vertical alignment (bits 2-3) */
	PXL_V_TOP    = 0,
	PXL_V_CENTER = 4,  /* 1 << 2 */
	PXL_V_BOTTOM = 8,  /* 2 << 2 */
} pxl_align_t;

/* Flip flags for transformations (bitmask/buffer flip, drawing, etc.).
 * Can be combined with |. Used by blit.h (rendering). */
typedef enum {
	PXL_FLIP_NONE = 0,    /* No flip */
	PXL_FLIP_H    = 1 << 0, /* Flip horizontally */
	PXL_FLIP_V    = 1 << 1, /* Flip vertically */
} pxl_flip_t;

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

	/* Assert to prevent integer overflow in edge calculations */
	assert(in.x <= INT_MAX - in.w);
	assert(bounds.x <= INT_MAX - bounds.w);
	assert(in.y <= INT_MAX - in.h);
	assert(bounds.y <= INT_MAX - bounds.h);

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

	/* Assert to prevent integer overflow in edge calculations */
	assert(in.x <= INT_MAX - in.w);
	assert(bounds.x <= INT_MAX - bounds.w);

	out->w = pxl_min(in.x + in.w, bounds.x + bounds.w) - out->x;
	
	return out->w > 0;
}

/* Align rect within container. Returns the aligned rect with same dimensions.
 * Use bitmask to combine horizontal and vertical alignment:
 *   PXL_H_LEFT | PXL_V_TOP, PXL_H_CENTER | PXL_V_CENTER, etc.
 *
 * Example:
 *   pxl_rect_t bounds = pxl_text_bounds(&writer, "Hello");
 *   pxl_rect_t container = {0, 0, 800, 600};
 *   pxl_rect_t aligned = pxl_rect_align(bounds, container, PXL_H_CENTER | PXL_V_CENTER);
 */
static inline pxl_rect_t
pxl_rect_align(pxl_rect_t rect, pxl_rect_t container, pxl_align_t align) {
	/* Align horizontally (bits 0-1) */
	switch (align & 0x03) {
	case PXL_H_LEFT:
		rect.x = container.x;
		break;
	case PXL_H_CENTER:
		rect.x = container.x + (container.w - rect.w) / 2;
		break;
	case PXL_H_RIGHT:
		rect.x = container.x + container.w - rect.w;
		break;
	default:
		assert(0);
	}

	/* Align vertically (bits 2-3) */
	switch (align & 0x0C) {
	case PXL_V_TOP:
		rect.y = container.y;
		break;
	case PXL_V_CENTER:
		rect.y = container.y + (container.h - rect.h) / 2;
		break;
	case PXL_V_BOTTOM:
		rect.y = container.y + container.h - rect.h;
		break;
	default:
		assert(0);
	}

	return rect;
}

#endif /* PXL_GEOM_H */
