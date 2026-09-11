#ifndef PXL_BITMASK_H
#define PXL_BITMASK_H

#include <stdint.h>  /* for uint8_t */

/* 1-bit per pixel mask.  Data is packed by byte. */
typedef struct {
    const uint8_t *data;    /* bitmask data (read-only)    */
    int            width;   /* width in pixels (bits)      */
    int            height;  /* height in pixels (rows)     */
    int            stride;  /* row stride in bytes         */
} pxl_bitmask_t;

#endif /* PXL_BITMASK_H */
