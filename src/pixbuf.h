#ifndef PIXBUF_H
#define PIXBUF_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "err.h"
#include "geom.h"

typedef uint32_t pix_t;

/* Pixel buffer row alignment in pixels. Must be a power of 2 for stride calculation.
   Default: 4 (optimized for 32-bit pixel access). Can be overridden before including this header. */
#ifndef PXL_ALIGN
#define PXL_ALIGN 4
#endif
/* Compile-time check: PXL_ALIGN must be a power of 2 (enables bitmask-based stride calculation) */
typedef char static_assert_pxl_align_is_power_of_2[(PXL_ALIGN & (PXL_ALIGN - 1)) == 0 ? 1 : -1];

typedef struct {
	pix_t  *data;    /* buffer data                     */
	int     width;   /* actual width in pix             */
	int     height;  /* actual height in pix            */
	int     stride;  /* row stride in pix for alignment */
} pixbuf_t;

static inline rect_t
pixbuf_bounds(const pixbuf_t *pb) {
	assert(pb);
	
	return (rect_t){0, 0, pb->width, pb->height};
}

static inline pix_t *
pixbuf_ptr(const pixbuf_t *pb, int x, int y) {
	assert(pb && pb->data);
	assert(x >= 0 && x < pb->width);
	assert(y >= 0 && y < pb->height);

	return pb->data + x + (y * pb->stride);
}

static inline pxl_err_t
pixbuf_init(pixbuf_t *pb, int w, int h) {
	if (w <= 0 || h <= 0) {
		return PXL_E_INVALID_PARAM;
	}

	pb->width  = w;
	pb->height = h;
	pb->stride = (w + PXL_ALIGN - 1) & ~(PXL_ALIGN - 1);

	pb->data   = malloc(pb->stride * pb->height * sizeof(pix_t));
	if (pb->data == NULL) {
		return PXL_E_OUT_OF_MEM;
	}

	return PXL_SUCCESS;
}

static inline void
pixbuf_deinit(pixbuf_t *pb) {
	if (pb->data) {
		free(pb->data);
		pb->data = NULL;
	}
}

#endif /* PIXBUF_H */
