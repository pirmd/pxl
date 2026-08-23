#ifndef PXL_BUF_H
#define PXL_BUF_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t pxl_t;  /* 32-bit ARGB pixel */

/*
 * PXL_BUF_ALIGN: Recommended pixel buffer alignment (power of 2).
 * Users are responsible for ensuring buffer stride is aligned.
 * Use pxl_calc_stride() to help compute a properly aligned stride.
 * Misaligned buffers may cause performance issues or crashes on some architectures (e.g., ARM).
 */

#ifndef PXL_BUF_ALIGN
#   define PXL_BUF_ALIGN 4
#endif

typedef char static_assert_pxl_buf_align_is_power_of_2[
    (PXL_BUF_ALIGN & (PXL_BUF_ALIGN - 1)) == 0 ? 1 : -1];

typedef struct {
	pxl_t  *data;    /* buffer data                     */
	int     width;   /* actual width in pix             */
	int     height;  /* actual height in pix            */
	int     stride;  /* row stride in pix for alignment */
} pxl_buf_t;

/* Calculate aligned stride for a buffer width */
static inline int
pxl_calc_stride(int w) {
	return (w + PXL_BUF_ALIGN - 1) & ~(PXL_BUF_ALIGN - 1);
}

/* Return address of pixel (x, y) in provided pixel buffer
 * Users are responsible to ensure that (x, y) lies in buffer bounds
 */
static inline pxl_t *
pxl_buf_ptr(const pxl_buf_t *pb, int x, int y) {
	assert(pb && pb->data);
	assert(x >= 0 && x < pb->width);
	assert(y >= 0 && y < pb->height);

	return pb->data + x + (y * pb->stride);
}

#endif
