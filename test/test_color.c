#include <assert.h>

#include "pxl.h"

/* pxl_t is a packed ARGB8888 pixel VALUE (0xAARRGGBB) on every platform:
 * the backends hand raw pxl_t buffers to X11 (visual masks 0x00ff0000/...)
 * and SDL (SDL_PIXELFORMAT_ARGB8888), so the value — not the memory byte
 * order — carries the channels. Regression test: the little-endian branch
 * once packed 0xBBGGRRAA, which made the X11 window read alpha as blue
 * (everything rendered violet) while accessor-based offscreen captures
 * looked correct. */
static void
test_argb_packed_value(void) {
    pxl_t c = pxl_argb(0xFF, 0x2A, 0x4D, 0x8F);
    assert(c == 0xFF2A4D8FU);
    assert(pxl_a(c) == 0xFF);
    assert(pxl_r(c) == 0x2A);
    assert(pxl_g(c) == 0x4D);
    assert(pxl_b(c) == 0x8F);
}

/* Raw literals follow the same convention (as test_blit.c already uses):
 * 0xFFFF0000 is red, 0xFF0000FF is blue. */
static void
test_raw_literals_read_back(void) {
    assert(pxl_r(0xFFFF0000U) == 0xFF && pxl_g(0xFFFF0000U) == 0x00 &&
           pxl_b(0xFFFF0000U) == 0x00 && pxl_a(0xFFFF0000U) == 0xFF);
    assert(pxl_r(0xFF0000FFU) == 0x00 && pxl_b(0xFF0000FFU) == 0xFF);
}

/* Round-trip over a spread of values, all channels distinct. */
static void
test_round_trip(void) {
    static const uint8_t ch[5] = { 0x00, 0x01, 0x80, 0xFE, 0xFF };
    for (int a = 0; a < 5; a++)
        for (int r = 0; r < 5; r++)
            for (int g = 0; g < 5; g++)
                for (int b = 0; b < 5; b++) {
                    pxl_t c = pxl_argb(ch[a], ch[r], ch[g], ch[b]);
                    assert(pxl_a(c) == ch[a] && pxl_r(c) == ch[r] &&
                           pxl_g(c) == ch[g] && pxl_b(c) == ch[b]);
                }
}

int
main(void) {
    test_argb_packed_value();
    test_raw_literals_read_back();
    test_round_trip();
    return 0;
}
