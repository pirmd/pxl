#ifndef PXL_FONT_ASCII_H
#define PXL_FONT_ASCII_H

#include "canvas.h"
#include "geom.h"  /* for pxl_rect_t */

/*
 * Draw a single ASCII character (0-127) at (x, y) on canvas using the built-in 8x8 bitmap font.
 * Non-printable chars (< 32) are replaced by space, out-of-range chars (> 127) by '?'.
 * Respects canvas offset and scissor.
 */
void
pxl_draw_char(pxl_canvas_t *cnv, int x, int y, unsigned char c);

/*
 * Draw an ASCII string at (x, y) on canvas.
 * Handles newlines ('\n') by advancing to the next line (y += advance_y).
 * Handles tabs ('\t') by advancing to the next tab stop (x += tab_width * advance_x).
 * Respects canvas offset and scissor.
 */
void
pxl_draw_str(pxl_canvas_t *cnv, int x, int y, const char *str);

/*
 * Return the bounding box of a single character (relative to (0,0)).
 * Handles special cases (< 32 -> ' ', > 127 -> '?') like pxl_draw_char.
 */
pxl_rect_t
pxl_char_bounds(unsigned char c);

/*
 * Return the bounding box of a string (relative to (0,0)).
 * Accounts for newlines: height increases by advance_y for each '\n'.
 * Accounts for tabs: width advances to next tab stop for each '\t'.
 * Width is the max x position reached (last char's right edge).
 * Returns {0,0,0,0} for empty strings.
 */
pxl_rect_t
pxl_str_bounds(const char *str);

#endif /* PXL_FONT_ASCII_H */
