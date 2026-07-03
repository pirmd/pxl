#ifndef PXL_COLOR_H
#define PXL_COLOR_H

#include <stdint.h>
#include "buf.h"

/* Only little-endian is supported (A: bits 24-31, R: 16-23, G: 8-15, B: 0-7) */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "native color's helpers only supports little-endian architectures"
#endif

/* Create color in ARGB8888 format (standard PXL format) */
static inline pxl_t
pxl_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* Extract color components from ARGB8888 pixel */
static inline uint8_t
pxl_a(pxl_t c) {
    return (c >> 24) & 0xFF;
}
static inline uint8_t
pxl_r(pxl_t c) {
    return (c >> 16) & 0xFF;
}
static inline uint8_t
pxl_g(pxl_t c) {
    return (c >> 8) & 0xFF;
}
static inline uint8_t
pxl_b(pxl_t c) {
    return c & 0xFF;
}

#endif /* PXL_COLOR_H */
