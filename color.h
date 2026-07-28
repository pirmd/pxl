#ifndef PXL_COLOR_H
#define PXL_COLOR_H

#include <stdint.h>
#include "buf.h"

/*
 * PXL uses ARGB8888 pixel format (A: bits 24-31, R: 16-23, G: 8-15, B: 0-7).
 * Memory layout depends on endianness:
 *   - Little-endian: [B, G, R, A] (byte 0 = B, byte 1 = G, byte 2 = R, byte 3 = A)
 *   - Big-endian:   [A, R, G, B] (byte 0 = A, byte 1 = R, byte 2 = G, byte 3 = B)
 * Use pxl_argb/pxl_a/pxl_r/pxl_g/pxl_b to ensure portability.
 * Direct bit manipulation on pxl_t is NOT portable across endianness.
 */

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

/* Big-endian implementation (ARGB in memory: [A, R, G, B]) */
static inline pxl_t
pxl_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline uint8_t pxl_a(pxl_t c) { return (c >> 24) & 0xFF; }
static inline uint8_t pxl_r(pxl_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t pxl_g(pxl_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t pxl_b(pxl_t c) { return c & 0xFF; }

#else

/* Little-endian implementation (ARGB in memory: [B, G, R, A]) */
static inline pxl_t
pxl_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)b << 24) | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)a;
}

static inline uint8_t pxl_a(pxl_t c) { return c & 0xFF; }
static inline uint8_t pxl_r(pxl_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t pxl_g(pxl_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t pxl_b(pxl_t c) { return (c >> 24) & 0xFF; }

#endif

#endif /* PXL_COLOR_H */
