#ifndef PXL_LAYOUT_H
#define PXL_LAYOUT_H

#include "geom.h"

/* Alignment flags */
typedef enum {
    PXL_ALIGN_LEFT     = (1 << 0),  /* Left alignment */
    PXL_ALIGN_H_CENTER = (1 << 1),  /* Horizontal center */
    PXL_ALIGN_RIGHT    = (1 << 2),  /* Right alignment */

    PXL_ALIGN_TOP      = (1 << 3),  /* Top alignment */
    PXL_ALIGN_V_CENTER = (1 << 4),  /* Vertical center */
    PXL_ALIGN_BOTTOM   = (1 << 5),  /* Bottom alignment */
} pxl_align_t;

/* Align a rectangle within container using PXL_ALIGN_* flags. */
static inline pxl_rect_t
pxl_align_rect(pxl_rect_t r, pxl_rect_t container, pxl_align_t align) {
    assert(r.x <= INT_MAX - r.w);
    assert(container.x <= INT_MAX - container.w);
    assert(r.y <= INT_MAX - r.h);
    assert(container.y <= INT_MAX - container.h);

    pxl_rect_t out_r = r;

    if (align & PXL_ALIGN_LEFT) {
        out_r.x = container.x;
    } else if (align & PXL_ALIGN_H_CENTER) {
        out_r.x = container.x + (container.w - r.w) / 2;
    } else if (align & PXL_ALIGN_RIGHT) {
        out_r.x = container.x + container.w - r.w;
    }

    if (align & PXL_ALIGN_TOP) {
        out_r.y = container.y;
    } else if (align & PXL_ALIGN_V_CENTER) {
        out_r.y = container.y + (container.h - r.h) / 2;
    } else if (align & PXL_ALIGN_BOTTOM) {
        out_r.y = container.y + container.h - r.h;
    }

    return out_r;
}

/* Side flags for rect division */
typedef enum {
    PXL_SIDE_LEFT     = (1 << 0),  /* Left side */
    PXL_SIDE_RIGHT    = (1 << 2),  /* Right side */
    PXL_SIDE_TOP      = (1 << 3),  /* Top side */
    PXL_SIDE_BOTTOM   = (1 << 5),  /* Bottom side */
} pxl_side_t;

/* Split a rectangle either vertically or horizontally. Modifies input rect. */
static inline pxl_rect_t
pxl_split_rect(pxl_rect_t *r, int d, pxl_side_t side) {
    assert(d >= 0);
    assert(r);
    assert(r->x <= INT_MAX - r->w);
    assert(r->y <= INT_MAX - r->h);

    pxl_rect_t out_r = *r;

    switch (side) {
    case PXL_SIDE_LEFT:
        if (d > r->w) d = r->w;
        out_r.w = d;
        r->x += d;
        r->w -= d;
        break;

    case PXL_SIDE_RIGHT:
        if (d > r->w) d = r->w;
        out_r.x = (r->x + r->w) - d;
        out_r.w = d;
        r->w -= d;
        break;

    case PXL_SIDE_TOP:
        if (d > r->h) d = r->h;
        out_r.h = d;
        r->y += d;
        r->h -= d;
        break;

    case PXL_SIDE_BOTTOM:
        if (d > r->h) d = r->h;
        out_r.y = (r->y + r->h) - d;
        out_r.h = d;
        r->h -= d;
        break;
    }

    return out_r;
}

/* Return a rectangle with padding applied. Positive values shrink, negative grow. */
static inline pxl_rect_t
pxl_pad_rect(pxl_rect_t r, int pad_w, int pad_h) {
    assert(r.x <= INT_MAX - r.w);
    assert(r.y <= INT_MAX - r.h);
    assert(pad_w <= INT_MAX / 2);
    assert(pad_h <= INT_MAX / 2);

    int out_w = r.w - 2 * pad_w;
    int out_h = r.h - 2 * pad_h;

    return (pxl_rect_t){
        .x = (out_w > 0) ? r.x + pad_w : r.x + r.w / 2,
        .y = (out_h > 0) ? r.y + pad_h : r.y + r.h / 2,
        .w = (out_w > 0) ? out_w : 0,
        .h = (out_h > 0) ? out_h : 0,
    };
}

#endif /* PXL_LAYOUT_H */
