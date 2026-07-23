#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pxl.h"

#define W 800
#define H 600
#define FPS 60.0f

#define BG_COLOR     0xFFFAFAFA  /* Off-white / light gray background */
#define TEXT_COLOR   0xFF000000  /* Black text for better contrast */
#define CURSOR_COLOR 0xFFFFFF00

/* margin and padding */
#define MARGIN_X 40
#define MARGIN_Y 40
#define LINE_SPACING 2

/* maximum text buffer size */
#define MAX_TEXT_LEN 1024

/* cursor blink timing */
#define CURSOR_BLINK_INTERVAL 0.7

/* Typewriter settings */
#define PAGE_WIDTH_CHARS 80          /* Standard typewriter page width */
#define CHAR_APPPEAR_DELAY 0.05     /* Delay between character appearances (seconds) */

/* Page margins (pixels) */
#define PAGE_MARGIN_LEFT  (MARGIN_X)
#define PAGE_MARGIN_RIGHT (W - MARGIN_X)

// Typewriter font data - 8x8 monochrome bitmap
// Based on IBM VGA fonts, modified for typewriter appearance
static char font_typewriter_data[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

/* Typewriter font using custom bitmap data */
static pxl_font_t pxl_font_typewriter = {
        .bitmask = {
            .data   = (const uint8_t *)font_typewriter_data,
            .width  = 8,
            .height = 128 * 8,
            .stride = 1
        },
        .rune_start = 0,
        .rune_end   = 127,
        .fallback_rune = '?',
        .tracking = 1,
        .leading = 8 + LINE_SPACING,
        .glyph_height = 8,
        .glyph_widths = NULL,
        .glyph_advances = NULL,
        .glyph_offsets_x = NULL,
        .glyph_offsets_y = NULL,
    };

typedef struct {
    uint32_t buffer[MAX_TEXT_LEN];
    double char_appear_time[MAX_TEXT_LEN]; /* Time when each char appeared */
    int length;
    int cursor_pos;
    int scroll_line;
    bool cursor_visible;
    double cursor_timer;
    
    /* Typewriter-specific */
    int margin_left;    /* Left margin in pixels */
    int margin_right;   /* Right margin in pixels */
    int char_width;     /* Width of one character in pixels */
    bool needs_carriage_return; /* Flag for manual carriage return */
} typewriter_t;

static void
update_cursor_blink(typewriter_t *tw, double dt) {
    tw->cursor_timer += dt;
    if (tw->cursor_timer >= CURSOR_BLINK_INTERVAL) {
        tw->cursor_timer -= CURSOR_BLINK_INTERVAL;
        tw->cursor_visible = !tw->cursor_visible;
    }
}

static void
insert_char(typewriter_t *tw, int c, double now) {
    if (tw->length >= MAX_TEXT_LEN - 1) {
        return; /* buffer full */
    }
    
    /* Typewriter: Auto line feed at right margin */
    /* Count characters on current line from cursor position */
    int chars_from_start = 0;
    int line_start_pos = tw->cursor_pos;
    while (line_start_pos > 0 && tw->buffer[line_start_pos - 1] != '\n') {
        line_start_pos--;
        chars_from_start++;
    }
    
    /* Check if adding this char would exceed margin */
    if (chars_from_start + 1 > PAGE_WIDTH_CHARS) {
        /* Insert line feed before this character */
        if (tw->length >= MAX_TEXT_LEN - 1) {
            return; /* buffer full */
        }
        
        /* Shift characters to the right of cursor for the newline */
        if (tw->cursor_pos < tw->length) {
            memmove(&tw->buffer[tw->cursor_pos + 1], 
                    &tw->buffer[tw->cursor_pos], 
                    (tw->length - tw->cursor_pos) * sizeof(uint32_t));
            memmove(&tw->char_appear_time[tw->cursor_pos + 1], 
                    &tw->char_appear_time[tw->cursor_pos], 
                    (tw->length - tw->cursor_pos) * sizeof(double));
        }
        
        tw->buffer[tw->cursor_pos] = '\n';
        tw->char_appear_time[tw->cursor_pos] = now;
        tw->length++;
        tw->cursor_pos++;
        tw->buffer[tw->length] = 0;
        tw->char_appear_time[tw->length] = 0;
        
        /* After inserting newline, recompute position */
        chars_from_start = 0;
    }
    
    /* Shift characters to the right of cursor for the new character */
    if (tw->cursor_pos < tw->length) {
        memmove(&tw->buffer[tw->cursor_pos + 1], 
                &tw->buffer[tw->cursor_pos], 
                (tw->length - tw->cursor_pos) * sizeof(uint32_t));
        memmove(&tw->char_appear_time[tw->cursor_pos + 1], 
                &tw->char_appear_time[tw->cursor_pos], 
                (tw->length - tw->cursor_pos) * sizeof(double));
    }
    
    tw->buffer[tw->cursor_pos] = (uint32_t)c;
    /* Typewriter effect: characters appear with a delay based on their position in the text */
    tw->char_appear_time[tw->cursor_pos] = now + (tw->length * CHAR_APPPEAR_DELAY);
    tw->length++;
    tw->cursor_pos++;
    tw->buffer[tw->length] = 0;
    tw->char_appear_time[tw->length] = 0;
}

static void
delete_char(typewriter_t *tw) {
    if (tw->cursor_pos > 0 && tw->length > 0) {
        /* Shift characters to the left of cursor */
        memmove(&tw->buffer[tw->cursor_pos - 1], 
                &tw->buffer[tw->cursor_pos], 
                (tw->length - tw->cursor_pos) * sizeof(uint32_t));
        memmove(&tw->char_appear_time[tw->cursor_pos - 1], 
                &tw->char_appear_time[tw->cursor_pos], 
                (tw->length - tw->cursor_pos) * sizeof(double));
        tw->length--;
        tw->cursor_pos--;
        tw->buffer[tw->length] = 0;
        tw->char_appear_time[tw->length] = 0;
    }
}

/* Return carriage to start of current line (typewriter behavior) */
static void
carriage_return(typewriter_t *tw) {
    /* Find start of current line */
    int line_start = tw->cursor_pos;
    while (line_start > 0 && tw->buffer[line_start - 1] != '\n') {
        line_start--;
    }
    tw->cursor_pos = line_start;
    tw->needs_carriage_return = true;
}

static void
move_cursor_left(typewriter_t *tw) {
    if (tw->cursor_pos > 0) {
        tw->cursor_pos--;
    }
}

static void
move_cursor_right(typewriter_t *tw) {
    if (tw->cursor_pos < tw->length) {
        tw->cursor_pos++;
    }
}

/* Keyboard layout type */
typedef enum {
    KEYBOARD_QWERTY,
    KEYBOARD_AZERTY
} keyboard_layout_t;

/* Common input handling (control keys, navigation) */
static void
handle_input_common(typewriter_t *tw, pxl_input_t *in) {
    /* Backspace */
    if (pxl_input_was_pressed(in, PXL_KEYB_BACKSPACE)) {
        delete_char(tw);
    }
    
    /* Delete */
    if (pxl_input_was_pressed(in, PXL_KEYB_DELETE)) {
        if (tw->cursor_pos < tw->length) {
            memmove(&tw->buffer[tw->cursor_pos], 
                    &tw->buffer[tw->cursor_pos + 1], 
                    tw->length - tw->cursor_pos);
            tw->length--;
            tw->buffer[tw->length] = '\0';
        }
    }
    
    /* Left arrow */
    if (pxl_input_was_pressed(in, PXL_KEYB_LEFT)) {
        move_cursor_left(tw);
    }
    
    /* Right arrow */
    if (pxl_input_was_pressed(in, PXL_KEYB_RIGHT)) {
        move_cursor_right(tw);
    }
    
    /* Home */
    if (pxl_input_was_pressed(in, PXL_KEYB_HOME)) {
        tw->cursor_pos = 0;
    }
    
    /* End */
    if (pxl_input_was_pressed(in, PXL_KEYB_END)) {
        tw->cursor_pos = tw->length;
    }
}

/* QWERTY layout input handler */
static void
handle_input_qwerty(typewriter_t *tw, pxl_input_t *in, double now) {
    handle_input_common(tw, in);
    
    /* Symbols (punctuation) */
    if (pxl_input_was_pressed(in, PXL_KEYB_SPACE))        { insert_char(tw, ' ', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_APOSTROPHE))   { insert_char(tw, '\'', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_COMMA))        { insert_char(tw, ',', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_MINUS))        { insert_char(tw, '-', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_PERIOD))       { insert_char(tw, '.', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_SLASH))        { insert_char(tw, '/', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_SEMICOLON))    { insert_char(tw, ';', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_EQUAL))         { insert_char(tw, '=', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_LEFT_BRACKET)) { insert_char(tw, '[', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_BACKSLASH))    { insert_char(tw, '\\', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_RIGHT_BRACKET)){ insert_char(tw, ']', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_GRAVE_ACCENT)) { insert_char(tw, '`', now); }
    
    /* Numbers */
    if (pxl_input_was_pressed(in, PXL_KEYB_0)) { insert_char(tw, '0', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_1)) { insert_char(tw, '1', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_2)) { insert_char(tw, '2', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_3)) { insert_char(tw, '3', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_4)) { insert_char(tw, '4', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_5)) { insert_char(tw, '5', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_6)) { insert_char(tw, '6', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_7)) { insert_char(tw, '7', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_8)) { insert_char(tw, '8', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_9)) { insert_char(tw, '9', now); }
    
    /* Letters - handle shift for uppercase */
    bool shift_pressed = pxl_input_is_pressed(in, PXL_KEYB_LSHIFT) || pxl_input_is_pressed(in, PXL_KEYB_RSHIFT);
    if (pxl_input_was_pressed(in, PXL_KEYB_A)) { insert_char(tw, shift_pressed ? 'A' : 'a', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_B)) { insert_char(tw, shift_pressed ? 'B' : 'b', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_C)) { insert_char(tw, shift_pressed ? 'C' : 'c', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_D)) { insert_char(tw, shift_pressed ? 'D' : 'd', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_E)) { insert_char(tw, shift_pressed ? 'E' : 'e', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_F)) { insert_char(tw, shift_pressed ? 'F' : 'f', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_G)) { insert_char(tw, shift_pressed ? 'G' : 'g', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_H)) { insert_char(tw, shift_pressed ? 'H' : 'h', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_I)) { insert_char(tw, shift_pressed ? 'I' : 'i', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_J)) { insert_char(tw, shift_pressed ? 'J' : 'j', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_K)) { insert_char(tw, shift_pressed ? 'K' : 'k', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_L)) { insert_char(tw, shift_pressed ? 'L' : 'l', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_M)) { insert_char(tw, shift_pressed ? 'M' : 'm', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_N)) { insert_char(tw, shift_pressed ? 'N' : 'n', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_O)) { insert_char(tw, shift_pressed ? 'O' : 'o', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_P)) { insert_char(tw, shift_pressed ? 'P' : 'p', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_Q)) { insert_char(tw, shift_pressed ? 'Q' : 'q', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_R)) { insert_char(tw, shift_pressed ? 'R' : 'r', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_S)) { insert_char(tw, shift_pressed ? 'S' : 's', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_T)) { insert_char(tw, shift_pressed ? 'T' : 't', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_U)) { insert_char(tw, shift_pressed ? 'U' : 'u', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_V)) { insert_char(tw, shift_pressed ? 'V' : 'v', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_W)) { insert_char(tw, shift_pressed ? 'W' : 'w', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_X)) { insert_char(tw, shift_pressed ? 'X' : 'x', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_Y)) { insert_char(tw, shift_pressed ? 'Y' : 'y', now); }
    if (pxl_input_was_pressed(in, PXL_KEYB_Z)) { insert_char(tw, shift_pressed ? 'Z' : 'z', now); }
    
    /* Enter */
    if (pxl_input_was_pressed(in, PXL_KEYB_ENTER)) {
        insert_char(tw, '\n', now);
    }
    
    /* Manual carriage return (Typewriter-specific) */
    if (pxl_input_was_pressed(in, PXL_KEYB_R)) {
        carriage_return(tw);
    }
}

/* AZERTY layout input handler */
static void
handle_input_azerty(typewriter_t *tw, pxl_input_t *in, double now) {
    handle_input_common(tw, in);
    
    bool shift_pressed = pxl_input_is_pressed(in, PXL_KEYB_LSHIFT) || pxl_input_is_pressed(in, PXL_KEYB_RSHIFT);
    bool altgr_pressed = pxl_input_is_pressed(in, PXL_KEYB_RALT);
    
    /* Enter */
    if (pxl_input_was_pressed(in, PXL_KEYB_ENTER)) {
        insert_char(tw, '\n', now);
        return;
    }
    
    /* Manual carriage return (Typewriter-specific) */
    if (pxl_input_was_pressed(in, PXL_KEYB_R)) {
        carriage_return(tw);
        return;
    }
    
    /* AZERTY: AltGr characters */
    if (altgr_pressed) {
        if (pxl_input_was_pressed(in, PXL_KEYB_O))     { insert_char(tw, 0x0153, now); }  /* œ */
        if (pxl_input_was_pressed(in, PXL_KEYB_A))     { insert_char(tw, 0x00E6, now); }  /* æ */
        if (pxl_input_was_pressed(in, PXL_KEYB_E))     { insert_char(tw, 0x20AC, now); }  /* € */
        if (pxl_input_was_pressed(in, PXL_KEYB_M))     { insert_char(tw, 0x00B5, now); }  /* µ */
        if (pxl_input_was_pressed(in, PXL_KEYB_8))     { insert_char(tw, '[', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_MINUS)) { insert_char(tw, ']', now); }
        return;
    }
    
    /* AZERTY: Numbers row with accented characters and symbols */
    if (shift_pressed) {
        /* Shift: Uppercase accented and symbols */
        if (pxl_input_was_pressed(in, PXL_KEYB_2))        { insert_char(tw, 0x00C9, now); }  /* É */
        if (pxl_input_was_pressed(in, PXL_KEYB_7))        { insert_char(tw, 0x00C8, now); }  /* È */
        if (pxl_input_was_pressed(in, PXL_KEYB_9))        { insert_char(tw, 0x00C7, now); }  /* Ç */
        if (pxl_input_was_pressed(in, PXL_KEYB_0))        { insert_char(tw, 0x00C0, now); }  /* À */
        if (pxl_input_was_pressed(in, PXL_KEYB_EQUAL))    { insert_char(tw, 0x00D9, now); }  /* Ù */
        /* Shift symbols */
        if (pxl_input_was_pressed(in, PXL_KEYB_3))        { insert_char(tw, '#', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_4))        { insert_char(tw, '{', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_5))        { insert_char(tw, '[', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_6))        { insert_char(tw, '|', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_8))        { insert_char(tw, '\\', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_APOSTROPHE)) { insert_char(tw, '~', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_EQUAL))    { insert_char(tw, '+', now); }
    } else {
        /* No shift: Lowercase accented and symbols */
        if (pxl_input_was_pressed(in, PXL_KEYB_2))        { insert_char(tw, 0x00E9, now); }  /* é */
        if (pxl_input_was_pressed(in, PXL_KEYB_7))        { insert_char(tw, 0x00E8, now); }  /* è */
        if (pxl_input_was_pressed(in, PXL_KEYB_9))        { insert_char(tw, 0x00E7, now); }  /* ç */
        if (pxl_input_was_pressed(in, PXL_KEYB_0))        { insert_char(tw, 0x00E0, now); }  /* à */
        if (pxl_input_was_pressed(in, PXL_KEYB_EQUAL))    { insert_char(tw, 0x00F9, now); }  /* ù */
        /* Symbols */
        if (pxl_input_was_pressed(in, PXL_KEYB_SPACE))       { insert_char(tw, ' ', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_APOSTROPHE))  { insert_char(tw, 0x00B2, now); }  /* ² */
        if (pxl_input_was_pressed(in, PXL_KEYB_3))         { insert_char(tw, '"', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_4))         { insert_char(tw, '\'', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_5))         { insert_char(tw, '(', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_6))         { insert_char(tw, '-', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_8))         { insert_char(tw, '_', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_BACKSLASH)) { insert_char(tw, 0x00B0, now); }  /* ° */
    }
    
    /* AZERTY: Letters */
    /* AZERTY physical layout: A<->Q swapped, Z<->W swapped, M is on COMMA key */
    if (shift_pressed) {
        /* Uppercase letters */
        /* AZERTY: Q key -> A, A key -> Q */
        if (pxl_input_was_pressed(in, PXL_KEYB_Q)) { insert_char(tw, 'A', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_A)) { insert_char(tw, 'Q', now); }
        /* AZERTY: W key -> Z, Z key -> W */
        if (pxl_input_was_pressed(in, PXL_KEYB_W)) { insert_char(tw, 'Z', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_Z)) { insert_char(tw, 'W', now); }
        /* Standard letters */
        if (pxl_input_was_pressed(in, PXL_KEYB_B)) { insert_char(tw, 'B', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_C)) { insert_char(tw, 'C', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_D)) { insert_char(tw, 'D', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_E)) { insert_char(tw, 'E', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_F)) { insert_char(tw, 'F', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_G)) { insert_char(tw, 'G', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_H)) { insert_char(tw, 'H', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_I)) { insert_char(tw, 'I', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_J)) { insert_char(tw, 'J', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_K)) { insert_char(tw, 'K', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_L)) { insert_char(tw, 'L', now); }
        /* AZERTY: M is on COMMA key */
        if (pxl_input_was_pressed(in, PXL_KEYB_COMMA)) { insert_char(tw, 'M', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_N)) { insert_char(tw, 'N', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_O)) { insert_char(tw, 'O', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_P)) { insert_char(tw, 'P', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_R)) { insert_char(tw, 'R', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_S)) { insert_char(tw, 'S', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_T)) { insert_char(tw, 'T', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_U)) { insert_char(tw, 'U', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_V)) { insert_char(tw, 'V', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_X)) { insert_char(tw, 'X', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_Y)) { insert_char(tw, 'Y', now); }
    } else {
        /* Lowercase letters */
        /* AZERTY: Q key -> a, A key -> q */
        if (pxl_input_was_pressed(in, PXL_KEYB_Q)) { insert_char(tw, 'a', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_A)) { insert_char(tw, 'q', now); }
        /* AZERTY: W key -> z, Z key -> w */
        if (pxl_input_was_pressed(in, PXL_KEYB_W)) { insert_char(tw, 'z', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_Z)) { insert_char(tw, 'w', now); }
        /* Standard letters */
        if (pxl_input_was_pressed(in, PXL_KEYB_B)) { insert_char(tw, 'b', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_C)) { insert_char(tw, 'c', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_D)) { insert_char(tw, 'd', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_E)) { insert_char(tw, 'e', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_F)) { insert_char(tw, 'f', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_G)) { insert_char(tw, 'g', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_H)) { insert_char(tw, 'h', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_I)) { insert_char(tw, 'i', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_J)) { insert_char(tw, 'j', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_K)) { insert_char(tw, 'k', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_L)) { insert_char(tw, 'l', now); }
        /* AZERTY: M is on COMMA key */
        if (pxl_input_was_pressed(in, PXL_KEYB_COMMA)) { insert_char(tw, 'm', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_N)) { insert_char(tw, 'n', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_O)) { insert_char(tw, 'o', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_P)) { insert_char(tw, 'p', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_R)) { insert_char(tw, 'r', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_S)) { insert_char(tw, 's', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_T)) { insert_char(tw, 't', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_U)) { insert_char(tw, 'u', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_V)) { insert_char(tw, 'v', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_X)) { insert_char(tw, 'x', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_Y)) { insert_char(tw, 'y', now); }
        /* AZERTY symbols on letter row */
        if (pxl_input_was_pressed(in, PXL_KEYB_SEMICOLON)) { insert_char(tw, ';', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_SLASH)) { insert_char(tw, '/', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_PERIOD)) { insert_char(tw, '.', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_LEFT_BRACKET)) { insert_char(tw, '^', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_RIGHT_BRACKET)) { insert_char(tw, '$', now); }
        if (pxl_input_was_pressed(in, PXL_KEYB_GRAVE_ACCENT)) { insert_char(tw, '`', now); }
    }
}

/* Main input handler - delegates to layout-specific handler */
static void
handle_input(typewriter_t *tw, pxl_input_t *in, keyboard_layout_t layout, double now) {
    if (layout == KEYBOARD_AZERTY) {
        handle_input_azerty(tw, in, now);
    } else {
        handle_input_qwerty(tw, in, now);
    }
}

static void
draw_text_with_cursor(pxl_canvas_t *cnv, typewriter_t *tw, double now) {
    pxl_text_ctx_t w;
    pxl_text_ctx_init(&w, cnv, &pxl_font_typewriter);
    
    int visible_lines = (H - MARGIN_Y * 2) / pxl_font_typewriter.leading;
    int char_h = pxl_font_typewriter.glyph_height;
    
    /* Draw all visible lines and track cursor position */
    int line_count = 0;
    int line_char_start = 0;
    int cursor_line = 0;
    int cursor_x = tw->margin_left;
    bool cursor_on_visible_line = false;
    
    /* First, find cursor line */
    for (int i = 0; i < tw->cursor_pos && i < tw->length; i++) {
        if (tw->buffer[i] == '\n') {
            cursor_line++;
        }
    }
    
    /* Draw visible lines */
    for (int i = 0; i <= tw->length; i++) {
        /* Handle carriage return */
        if (tw->needs_carriage_return && line_count == cursor_line) {
            pxl_text_set_cursor(&w, tw->margin_left, w.y);
            tw->needs_carriage_return = false;
        }
        
        if (tw->buffer[i] == '\n' || tw->buffer[i] == '\0') {
            int line_end = (tw->buffer[i] == '\n') ? i : tw->length;
            
            if (line_count >= tw->scroll_line && line_count < tw->scroll_line + visible_lines) {
                int y_pos = MARGIN_Y + (line_count - tw->scroll_line) * pxl_font_typewriter.leading;
                pxl_text_set_cursor(&w, tw->margin_left, y_pos);
                
                for (int j = line_char_start; j < line_end; j++) {
                    /* Typewriter effect: only draw characters that have appeared */
                    if (now >= tw->char_appear_time[j]) {
                        /* Save cursor x position BEFORE drawing the character at cursor pos */
                        if (line_count == cursor_line && j == tw->cursor_pos) {
                            cursor_x = w.x;
                            cursor_on_visible_line = true;
                        }
                        
                        /* Use text color */
                        pxl_canvas_set_color(cnv, TEXT_COLOR);
                        pxl_draw_rune(&w, tw->buffer[j]);
                    }
                }
                
                /* Check if cursor is at end of this line (after last char) */
                if (line_count == cursor_line && tw->cursor_pos == line_end) {
                    cursor_x = w.x;
                    cursor_on_visible_line = true;
                }
            }
            
            line_count++;
            line_char_start = i + 1;
        }
    }
    
    /* Draw cursor if visible */
    if (tw->cursor_visible && cursor_on_visible_line) {
        int visible_line = cursor_line - tw->scroll_line;
        int cursor_y = MARGIN_Y + visible_line * pxl_font_typewriter.leading;
        
        pxl_canvas_set_color(cnv, CURSOR_COLOR);
        pxl_fill_rect(cnv, cursor_x, cursor_y, 2, char_h);
    }
}

static void
render_typewriter(pxl_canvas_t *cnv, typewriter_t *tw, double now) {
    /* Clear */
    pxl_canvas_set_color(cnv, BG_COLOR);
    pxl_canvas_clear(cnv);
    
    /* Draw text with cursor */
    draw_text_with_cursor(cnv, tw, now);
}

static void
init_typewriter(typewriter_t *tw) {
    memset(tw, 0, sizeof(*tw));
    tw->length = 0;
    tw->cursor_pos = 0;
    tw->scroll_line = 0;
    tw->cursor_visible = true;
    tw->cursor_timer = 0.0;
    
    /* Typewriter-specific initialization */
    tw->margin_left = PAGE_MARGIN_LEFT;
    tw->margin_right = PAGE_MARGIN_RIGHT;
    tw->char_width = 8; /* Font is 8px wide */
    tw->needs_carriage_return = false;
}

static void
update_fps(double now, int *current_fps) {
    static double t0 = 0;
    static int n = 0;
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        *current_fps = (int)((float)n / (float)(now - t0));
        n = 0;
        t0 = now;
    }
}

int
main(void) {
    if (pxl_backend_init("PXL Typewriter", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Typewriter demo. Type text, use arrows to navigate, Backspace/Delete to edit.\n");
    printf("F1=toggle AZERTY/QWERTY, R=carriage return. ESC=quit\n");

    typewriter_t tw;
    init_typewriter(&tw);

    pxl_input_t in;
    pxl_input_init(&in);

    int current_fps = 0;
    keyboard_layout_t layout = KEYBOARD_QWERTY;

    double prev_time = pxl_backend_get_time();
    while (!pxl_input_is_pressed(&in, PXL_KEYB_ESCAPE) && !pxl_input_pressed(&in.cur, PXL_WM_QUIT)) {
        double now = pxl_backend_get_time();
        double dt = now - prev_time;
        prev_time = now;
        
        pxl_input_next_state(&in);
        pxl_backend_poll_events(&in.cur);
        
        /* Toggle keyboard layout with F1 */
        if (pxl_input_was_pressed(&in, PXL_KEYB_F1)) {
            layout = (layout == KEYBOARD_QWERTY) ? KEYBOARD_AZERTY : KEYBOARD_QWERTY;
            printf("Keyboard layout: %s\n", layout == KEYBOARD_AZERTY ? "AZERTY" : "QWERTY");
        }
        
        handle_input(&tw, &in, layout, now);
        
        update_cursor_blink(&tw, dt);
        update_fps(now, &current_fps);

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);
            
            render_typewriter(&cnv, &tw, now);
            
            pxl_backend_end_frame();
        }
    }

    printf("Final text length: %d characters\n", tw.length);
    pxl_backend_deinit();
    return 0;
}
