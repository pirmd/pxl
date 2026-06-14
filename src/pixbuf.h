#ifndef PXL_PIXBUF_H
#define PXL_PIXBUF_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "err.h"

typedef uint32_t pxl_pixel_t;

#define PXL_ALIGN 4
/* PXL_ALIGN must be a power of 2 for stride calculation */
typedef char static_assert_pxl_align_is_power_of_2[(PXL_ALIGN & (PXL_ALIGN - 1)) == 0 ? 1 : -1];

typedef struct {
	pxl_pixel_t *data;
	size_t       width;
	size_t       height;
	size_t       stride;
} pixbuf_t;

static inline pxl_pixel_t *
pixbuf_ptr(const pixbuf_t *pb, int x, int y) {
	assert(pb && pb->data);
	assert(x >= 0 && (size_t)x < pb->width);
	assert(y >= 0 && (size_t)y < pb->height);

	uint8_t *row_ptr = (uint8_t *)pb->data + ((size_t)y * pb->stride * sizeof(pxl_pixel_t));
	return (pxl_pixel_t *)row_ptr + (size_t)(x);
}

static inline pxl_err_t
pixbuf_init(pixbuf_t *pb, int w, int h) {
	if (w <= 0 || h <= 0) {
		return PXL_E_INVALID_PARAM;
	}

	pb->width  = w;
	pb->height = h;
	pb->stride = (w + PXL_ALIGN - 1) & ~(PXL_ALIGN - 1);

	pb->data   = malloc(pb->stride * pb->height * sizeof(*pb->data));
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

#endif /* PXL_PIXBUF_H */
