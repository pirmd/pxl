/* buf.c - pixel buffer file export */

#include <stdio.h>

#include "buf.h"
#include "color.h"

int
pxl_buf_save_ppm(const pxl_buf_t *pb, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    fprintf(f, "P6\n%d %d\n255\n", pb->width, pb->height);
    for (int y = 0; y < pb->height; y++) {
        const pxl_t *row = pb->data + y * pb->stride;
        for (int x = 0; x < pb->width; x++) {
            fputc(pxl_r(row[x]), f);
            fputc(pxl_g(row[x]), f);
            fputc(pxl_b(row[x]), f);
        }
    }
    int err = ferror(f);
    fclose(f);
    return err ? -1 : 0;
}
