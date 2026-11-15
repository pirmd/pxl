#include <stddef.h>  /* for ptrdiff_t */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "buf.h"
#include "color.h"

/* pxl_calc_stride tests ------------------------------------------------------ */

static void
test_pxl_calc_stride_exact_multiples(void) {
    assert(pxl_calc_stride(0) == 0);
    assert(pxl_calc_stride(4) == 4);
    assert(pxl_calc_stride(8) == 8);
    assert(pxl_calc_stride(100) == 100);
}

static void
test_pxl_calc_stride_rounding_up(void) {
    assert(pxl_calc_stride(1) == 4);
    assert(pxl_calc_stride(5) == 8);
    assert(pxl_calc_stride(101) == 104);
    assert(pxl_calc_stride(99) == 100);
    assert(pxl_calc_stride(1000) == 1000);
    assert(pxl_calc_stride(1001) == 1004);
}

static void
test_pxl_calc_stride_alignment_guarantee(void) {
    for (int w = 0; w < 1024; w++) {
        int stride = pxl_calc_stride(w);
        assert(stride % PXL_BUF_ALIGN == 0);
        assert(stride >= w);
    }
}

/* Fixture for pxl_buf_ptr tests -------------------------------------------- */
#define FIXTURE_W 101
#define FIXTURE_H 10
#define FIXTURE_STRIDE 104  /* pxl_calc_stride(101) = 104 */

static pxl_t g_buf_data[FIXTURE_STRIDE * FIXTURE_H];
static pxl_buf_t g_buf = {
    .data = g_buf_data,
    .width = FIXTURE_W,
    .height = FIXTURE_H,
    .stride = FIXTURE_STRIDE
};

static inline void
fixture_reset(void) {
    memset(g_buf_data, 0x00, sizeof(g_buf_data));
}

/* pxl_buf_ptr tests -------------------------------------------------------- */

static void
test_pxl_buf_ptr_basic_access(void) {
    fixture_reset();

    pxl_t value = 0xCAFEBABE;
    pxl_t *ptr = pxl_buf_ptr(&g_buf, 3, 4);
    *ptr = value;
    assert(*pxl_buf_ptr(&g_buf, 3, 4) == value);
}

static void
test_pxl_buf_ptr_corners(void) {
    fixture_reset();

    /* Test all four corners */
    *pxl_buf_ptr(&g_buf, 0, 0) = 1;
    assert(*pxl_buf_ptr(&g_buf, 0, 0) == 1);

    *pxl_buf_ptr(&g_buf, g_buf.width - 1, 0) = 2;
    assert(*pxl_buf_ptr(&g_buf, g_buf.width - 1, 0) == 2);

    *pxl_buf_ptr(&g_buf, 0, g_buf.height - 1) = 3;
    assert(*pxl_buf_ptr(&g_buf, 0, g_buf.height - 1) == 3);

    *pxl_buf_ptr(&g_buf, g_buf.width - 1, g_buf.height - 1) = 4;
    assert(*pxl_buf_ptr(&g_buf, g_buf.width - 1, g_buf.height - 1) == 4);
}

static void
test_pxl_buf_ptr_minimal_buffer(void) {
    /* Test with minimal valid buffer (1x1) */
    pxl_t data[4];  /* stride=4 for width=1 */
    pxl_buf_t buf = {
        .data = data,
        .width = 1,
        .height = 1,
        .stride = 4
    };
    pxl_t *ptr = pxl_buf_ptr(&buf, 0, 0);
    assert(ptr == &data[0]);
    *ptr = 0xCAFEBABE;
    assert(*pxl_buf_ptr(&buf, 0, 0) == 0xCAFEBABE);
}

static void
test_pxl_buf_ptr_stride_calculation(void) {
    fixture_reset();

    /* Verify row pointer difference equals stride */
    pxl_t *ptr_row0 = pxl_buf_ptr(&g_buf, 0, 0);
    pxl_t *ptr_row1 = pxl_buf_ptr(&g_buf, 0, 1);
    ptrdiff_t diff = (char *)ptr_row1 - (char *)ptr_row0;
    assert(diff == (ptrdiff_t)((size_t)g_buf.stride * sizeof(pxl_t)));
}

static void
test_pxl_buf_ptr_grid_access(void) {
    fixture_reset();

    /* Fill all pixels with unique values */
    for (int y = 0; y < g_buf.height; y++) {
        for (int x = 0; x < g_buf.width; x++) {
            pxl_t *ptr = pxl_buf_ptr(&g_buf, x, y);
            *ptr = (pxl_t)(x + y * g_buf.width);
        }
    }

    /* Verify all pixels */
    for (int y = 0; y < g_buf.height; y++) {
        for (int x = 0; x < g_buf.width; x++) {
            assert(*pxl_buf_ptr(&g_buf, x, y) == (pxl_t)(x + y * g_buf.width));
        }
    }
}

/* pxl_buf_save_ppm tests ---------------------------------------------------- */

static const char *PPM_TMP = "test_buf_save_ppm_tmp.ppm";

static void
test_pxl_buf_save_ppm_roundtrip(void) {
    /* width 3 with stride 4: the padding column must not reach the file */
    pxl_t data[4 * 2];
    pxl_buf_t buf = { .data = data, .width = 3, .height = 2, .stride = 4 };
    for (int i = 0; i < 8; i++)
        data[i] = 0xFF000000;  /* padding marker, opaque black */
    *pxl_buf_ptr(&buf, 0, 0) = pxl_argb(0xFF, 1, 2, 3);
    *pxl_buf_ptr(&buf, 1, 0) = pxl_argb(0x80, 4, 5, 6);  /* alpha dropped */
    *pxl_buf_ptr(&buf, 2, 0) = pxl_argb(0xFF, 7, 8, 9);
    *pxl_buf_ptr(&buf, 0, 1) = pxl_argb(0xFF, 10, 11, 12);
    *pxl_buf_ptr(&buf, 1, 1) = pxl_argb(0xFF, 13, 14, 15);
    *pxl_buf_ptr(&buf, 2, 1) = pxl_argb(0xFF, 16, 17, 18);

    assert(pxl_buf_save_ppm(&buf, PPM_TMP) == 0);

    FILE *f = fopen(PPM_TMP, "rb");
    assert(f);
    const char *hdr = "P6\n3 2\n255\n";
    char header[32];
    assert(fread(header, 1, strlen(hdr), f) == strlen(hdr));
    assert(memcmp(header, hdr, strlen(hdr)) == 0);
    unsigned char px[3 * 2 * 3];
    assert(fread(px, 1, sizeof(px), f) == sizeof(px));
    assert(fgetc(f) == EOF);  /* no extra byte */
    fclose(f);

    const unsigned char expect[3 * 2 * 3] = {
        1, 2, 3,  4, 5, 6,  7, 8, 9,
        10, 11, 12,  13, 14, 15,  16, 17, 18,
    };
    assert(memcmp(px, expect, sizeof(px)) == 0);
    remove(PPM_TMP);
}

static void
test_pxl_buf_save_ppm_io_error(void) {
    pxl_t data[4];
    pxl_buf_t buf = { .data = data, .width = 1, .height = 1, .stride = 4 };
    assert(pxl_buf_save_ppm(&buf, "/nonexistent-dir-pxl/x.ppm") == -1);
}

/* Main -------------------------------------------------------------------- */

int
main(void) {
    test_pxl_calc_stride_exact_multiples();
    test_pxl_calc_stride_rounding_up();
    test_pxl_calc_stride_alignment_guarantee();

    test_pxl_buf_ptr_basic_access();
    test_pxl_buf_ptr_corners();
    test_pxl_buf_ptr_stride_calculation();
    test_pxl_buf_ptr_grid_access();
    test_pxl_buf_ptr_minimal_buffer();

    test_pxl_buf_save_ppm_roundtrip();
    test_pxl_buf_save_ppm_io_error();

    return 0;
}
