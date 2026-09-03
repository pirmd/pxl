#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bitmask.h"
#include "buf.h"
#include "transform.h"

/* =============================================================================
 * Buffer Flip Functions
 * =============================================================================
 */

void
pxl_buf_flip_h(pxl_buf_t *dst, const pxl_buf_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const pxl_t *src_row = pxl_buf_ptr(src, 0, y);
		pxl_t *dst_row = pxl_buf_ptr(dst, 0, y);

		for (int x = 0; x < src->width; x++) {
			dst_row[x] = src_row[src->width - 1 - x];
		}
	}
}

void
pxl_buf_flip_v(pxl_buf_t *dst, const pxl_buf_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const pxl_t *src_row = pxl_buf_ptr(src, 0, src->height - 1 - y);
		pxl_t *dst_row = pxl_buf_ptr(dst, 0, y);

		memcpy(dst_row, src_row, (size_t)src->width * sizeof(pxl_t));
	}
}

void
pxl_buf_flip_hv(pxl_buf_t *dst, const pxl_buf_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const pxl_t *src_row = pxl_buf_ptr(src, 0, src->height - 1 - y);
		pxl_t *dst_row = pxl_buf_ptr(dst, 0, y);

		for (int x = 0; x < src->width; x++) {
			dst_row[x] = src_row[src->width - 1 - x];
		}
	}
}

/* =============================================================================
 * Bitmask Flip Functions
 * =============================================================================
 */

void
pxl_bitmask_flip_h(pxl_bitmask_t *dst, const pxl_bitmask_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const uint8_t *src_row = src->data + (size_t)y * src->stride;
		uint8_t *dst_row = dst->data + (size_t)y * dst->stride;

		/* Process each byte, flipping bits within the byte */
		for (size_t byte_idx = 0; byte_idx < dst->stride; byte_idx++) {
			uint8_t src_byte = src_row[byte_idx];
			uint8_t flipped_byte = 0;

			/* Flip bits in the byte (LSB <-> MSB) */
			for (int bit = 0; bit < 8; bit++) {
				if (src_byte & (1U << bit)) {
					flipped_byte |= (1U << (7 - bit));
				}
			}

			/* If this is the last byte and width is not a multiple of 8,
			 * we need to mask unused bits */
			if (byte_idx == dst->stride - 1) {
				int bits_in_last_byte = src->width % 8;
				if (bits_in_last_byte == 0) bits_in_last_byte = 8;

				/* Create mask for valid bits */
				uint8_t mask = (1U << bits_in_last_byte) - 1;
				flipped_byte = (flipped_byte >> (8 - bits_in_last_byte)) & mask;
			}

			dst_row[byte_idx] = flipped_byte;
		}
	}
}

void
pxl_bitmask_flip_v(pxl_bitmask_t *dst, const pxl_bitmask_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const uint8_t *src_row = src->data + (size_t)(src->height - 1 - y) * src->stride;
		uint8_t *dst_row = dst->data + (size_t)y * dst->stride;

		memcpy(dst_row, src_row, dst->stride);
	}
}

void
pxl_bitmask_flip_hv(pxl_bitmask_t *dst, const pxl_bitmask_t *src) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(dst->width == src->width);
	assert(dst->height == src->height);

	for (int y = 0; y < src->height; y++) {
		const uint8_t *src_row = src->data + (size_t)(src->height - 1 - y) * src->stride;
		uint8_t *dst_row = dst->data + (size_t)y * dst->stride;

		/* Process each byte, flipping bits within the byte */
		for (size_t byte_idx = 0; byte_idx < dst->stride; byte_idx++) {
			uint8_t src_byte = src_row[byte_idx];
			uint8_t flipped_byte = 0;

			/* Flip bits in the byte (LSB <-> MSB) */
			for (int bit = 0; bit < 8; bit++) {
				if (src_byte & (1U << bit)) {
					flipped_byte |= (1U << (7 - bit));
				}
			}

			/* If this is the last byte and width is not a multiple of 8,
			 * we need to mask unused bits */
			if (byte_idx == dst->stride - 1) {
				int bits_in_last_byte = src->width % 8;
				if (bits_in_last_byte == 0) bits_in_last_byte = 8;

				/* Create mask for valid bits */
				uint8_t mask = (1U << bits_in_last_byte) - 1;
				flipped_byte = (flipped_byte >> (8 - bits_in_last_byte)) & mask;
			}

			dst_row[byte_idx] = flipped_byte;
		}
	}
}

/* =============================================================================
 * Buffer Scale Functions (Nearest-Neighbor)
 * =============================================================================
 */

void
pxl_buf_scale(pxl_buf_t *dst, const pxl_buf_t *src, int scale_x, int scale_y) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(scale_x >= 1);
	assert(scale_y >= 1);
	assert(dst->width == src->width * scale_x);
	assert(dst->height == src->height * scale_y);

	for (int src_y = 0; src_y < src->height; src_y++) {
		const pxl_t *src_row = pxl_buf_ptr(src, 0, src_y);

		for (int src_x = 0; src_x < src->width; src_x++) {
			pxl_t pixel = src_row[src_x];

			/* Fill the scaled region */
			int dst_x_start = src_x * scale_x;
			int dst_y_start = src_y * scale_y;
			int dst_x_end = dst_x_start + scale_x;
			int dst_y_end = dst_y_start + scale_y;

			for (int dst_y = dst_y_start; dst_y < dst_y_end; dst_y++) {
				pxl_t *dst_row = pxl_buf_ptr(dst, 0, dst_y);
				for (int dst_x = dst_x_start; dst_x < dst_x_end; dst_x++) {
					dst_row[dst_x] = pixel;
				}
			}
		}
	}
}

/* =============================================================================
 * Bitmask Scale Functions (Nearest-Neighbor)
 * =============================================================================
 */

void
pxl_bitmask_scale(pxl_bitmask_t *dst, const pxl_bitmask_t *src, int scale_x, int scale_y) {
	assert(dst && dst->data);
	assert(src && src->data);
	assert(scale_x >= 1);
	assert(scale_y >= 1);
	assert(dst->width == src->width * scale_x);
	assert(dst->height == src->height * scale_y);

	for (int src_y = 0; src_y < src->height; src_y++) {
		const uint8_t *src_row = src->data + (size_t)src_y * src->stride;

		for (int src_x = 0; src_x < src->width; src_x++) {
			/* Get the source bit */
			size_t byte_idx = (size_t)src_x / 8u;
			int bit_idx = src_x % 8;
			uint8_t src_byte = src_row[byte_idx];
			bool bit_set = (src_byte & (1U << bit_idx)) != 0;

			if (bit_set) {
				/* Fill the scaled region in destination */
				int dst_x_start = src_x * scale_x;
				int dst_y_start = src_y * scale_y;
				int dst_x_end = dst_x_start + scale_x;
				int dst_y_end = dst_y_start + scale_y;

				for (int dst_y = dst_y_start; dst_y < dst_y_end; dst_y++) {
					uint8_t *dst_row = dst->data + (size_t)dst_y * dst->stride;

					/* Calculate byte range to fill */
					int start_byte = dst_x_start / 8;
					int end_byte = (dst_x_end + 7) / 8;

					for (int byte = start_byte; byte < end_byte; byte++) {
						int byte_start_x = byte * 8;
						int byte_end_x = byte_start_x + 8;

						int fill_start_x = dst_x_start > byte_start_x ? dst_x_start : byte_start_x;
						int fill_end_x = dst_x_end < byte_end_x ? dst_x_end : byte_end_x;

						/* Create bitmask for this byte */
						uint8_t mask = 0;
						for (int x = fill_start_x; x < fill_end_x; x++) {
							int bit_pos = x % 8;
							mask |= (1U << bit_pos);
						}

						/* OR the mask into destination */
						dst_row[byte] |= mask;
					}
				}
			}
		}
	}
}
