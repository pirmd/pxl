#ifndef PXL_BITMASK_H
#define PXL_BITMASK_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/* 1-bit per pixel mask for stencil/clip operations.
   Data is packed in bytes (8 bits per byte). LSB of each byte is bit 0. */
typedef struct {
    const uint8_t *data;    /* bitmask data (read-only)    */
    int            width;   /* width in pixels (bits)      */
    int            height;  /* height in pixels (rows)     */
    size_t         stride;  /* row stride in BYTE          */
} pxl_bitmask_t;

#endif /* PXL_BITMASK_H */
