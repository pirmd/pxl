#ifndef PXL_COLOR_H
#define PXL_COLOR_H

#include <stdint.h>
#include "buf.h"

/*
 * pxl_t is a packed ARGB8888 pixel VALUE: A in bits 24-31, R in 16-23,
 * G in 8-15, B in 0-7 — the same numeric value on every platform, matching
 * what the backends hand to X11 (visual masks 0x00ff0000/...) and SDL
 * (SDL_PIXELFORMAT_ARGB8888) as raw pixels. Only the in-memory byte order
 * of that value follows endianness (LE: [B,G,R,A], BE: [A,R,G,B]); the
 * backends handle that, callers must not.
 * Use pxl_argb/pxl_a/pxl_r/pxl_g/pxl_b to build and read colors.
 * Direct bit manipulation on pxl_t is NOT portable across endianness.
 */
static inline pxl_t
pxl_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline uint8_t pxl_a(pxl_t c) { return (c >> 24) & 0xFF; }
static inline uint8_t pxl_r(pxl_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t pxl_g(pxl_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t pxl_b(pxl_t c) { return c & 0xFF; }

#endif /* PXL_COLOR_H */
