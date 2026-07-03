#ifndef PXL_BUF_H
#define PXL_BUF_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "err.h"

typedef uint32_t pxl_t;

/* Pixel buffer row alignment in pixels. Must be a power of 2 for stride calculation.
   Default: 4 (optimized for 32-bit pixel access). Can be overridden before including this header. */
#ifndef PXL_ALIGN
#    define PXL_ALIGN 4
#endif

typedef char static_assert_pxl_align_is_power_of_2[(PXL_ALIGN & (PXL_ALIGN - 1)) == 0 ? 1 : -1];

typedef struct {
	pxl_t  *data;    /* buffer data                     */
	int     width;   /* actual width in pix             */
	int     height;  /* actual height in pix            */
	int     stride;  /* row stride in pix for alignment */
} pxl_buf_t;

static inline pxl_t *
pxl_buf_ptr(const pxl_buf_t *pb, int x, int y) {
	assert(pb && pb->data);
	assert(x >= 0 && x < pb->width);
	assert(y >= 0 && y < pb->height);

	return pb->data + x + (y * pb->stride);
}

static inline pxl_err_t
pxl_buf_init(pxl_buf_t *pb, int w, int h) {
	if (w <= 0 || h <= 0) {
		return PXL_E_INVALID_PARAM;
	}

	pb->width  = w;
	pb->height = h;
	pb->stride = (w + PXL_ALIGN - 1) & ~(PXL_ALIGN - 1);

	pb->data   = malloc(pb->stride * pb->height * sizeof(pxl_t));
	if (pb->data == NULL) {
		return PXL_E_OUT_OF_MEM;
	}

	return PXL_SUCCESS;
}

static inline void
pxl_buf_deinit(pxl_buf_t *pb) {
	if (pb->data) {
		free(pb->data);
		pb->data = NULL;
	}
}

#endif /* PXL_BUF_H */
