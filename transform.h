#ifndef PXL_TRANSFORM_H
#define PXL_TRANSFORM_H

#include "bitmask.h"
#include "buf.h"

/* Flip a pixel buffer horizontally.
 *
 * Args:
 *   dst: Destination buffer (must be pre-allocated with same dimensions as src)
 *   src: Source buffer to flip
 *
 * Note: dst and src can be the same buffer (in-place flip).
 */
void pxl_buf_flip_h(pxl_buf_t *dst, const pxl_buf_t *src);

/* Flip a pixel buffer vertically.
 *
 * Args:
 *   dst: Destination buffer (must be pre-allocated with same dimensions as src)
 *   src: Source buffer to flip
 *
 * Note: dst and src can be the same buffer (in-place flip).
 */
void pxl_buf_flip_v(pxl_buf_t *dst, const pxl_buf_t *src);

/* Flip a pixel buffer both horizontally and vertically.
 *
 * Args:
 *   dst: Destination buffer (must be pre-allocated with same dimensions as src)
 *   src: Source buffer to flip
 *
 * Note: dst and src can be the same buffer (in-place flip).
 */
void pxl_buf_flip_hv(pxl_buf_t *dst, const pxl_buf_t *src);

/* Flip a bitmask horizontally.
 *
 * Args:
 *   dst: Destination bitmask (must be pre-allocated with same dimensions as src)
 *   src: Source bitmask to flip
 *
 * Note: dst and src can be the same bitmask (in-place flip).
 */
void pxl_bitmask_flip_h(pxl_bitmask_t *dst, const pxl_bitmask_t *src);

/* Flip a bitmask vertically.
 *
 * Args:
 *   dst: Destination bitmask (must be pre-allocated with same dimensions as src)
 *   src: Source bitmask to flip
 *
 * Note: dst and src can be the same bitmask (in-place flip).
 */
void pxl_bitmask_flip_v(pxl_bitmask_t *dst, const pxl_bitmask_t *src);

/* Flip a bitmask both horizontally and vertically.
 *
 * Args:
 *   dst: Destination bitmask (must be pre-allocated with same dimensions as src)
 *   src: Source bitmask to flip
 *
 * Note: dst and src can be the same bitmask (in-place flip).
 */
void pxl_bitmask_flip_hv(pxl_bitmask_t *dst, const pxl_bitmask_t *src);

/* Scale a pixel buffer using nearest-neighbor interpolation.
 *
 * Args:
 *   dst: Destination buffer (must be pre-allocated)
 *   src: Source buffer to scale
 *   scale_x: Horizontal scale factor (must be >= 1)
 *   scale_y: Vertical scale factor (must be >= 1)
 *
 * Note: For scale factors < 1, use a temporary buffer and flip the operation.
 */
void pxl_buf_scale(pxl_buf_t *dst, const pxl_buf_t *src, int scale_x, int scale_y);

/* Scale a bitmask using nearest-neighbor interpolation.
 *
 * Args:
 *   dst: Destination bitmask (must be pre-allocated)
 *   src: Source bitmask to scale
 *   scale_x: Horizontal scale factor (must be >= 1)
 *   scale_y: Vertical scale factor (must be >= 1)
 *
 * Note: For scale factors < 1, use a temporary buffer and flip the operation.
 */
void pxl_bitmask_scale(pxl_bitmask_t *dst, const pxl_bitmask_t *src, int scale_x, int scale_y);

#endif /* PXL_TRANSFORM_H */
