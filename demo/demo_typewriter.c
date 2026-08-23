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

#define MAX_TEXT_LEN 1024 /* maximum text buffer size */

/* Typewriter settings */
#define PAGE_WIDTH_CHARS 80

#define MARGIN_X 40
#define MARGIN_Y 40
#define PAGE_MARGIN_LEFT  (MARGIN_X)
#define PAGE_MARGIN_RIGHT (W - MARGIN_X)
#define LINE_SPACING 2

#define CHAR_APPPEAR_DELAY 0.05     /* Delay between character appearances (seconds) */

/* Typewriter 8x8 monochrome bitmap font based on IBM VGA fonts */
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
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 ("")
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
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\\)
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

typedef enum keyboard_layout_e {
    KEYBOARD_QWERTY,
    KEYBOARD_AZERTY
} keyboard_layout_t;

/*
 *  Typewriter state
 */
typedef struct typewriter_s {
    uint32_t buffer[MAX_TEXT_LEN];
    double char_appear_time[MAX_TEXT_LEN];
    int length;
    int cursor_pos;
    int scroll_line;
    bool cursor_visible;
    double cursor_timer;

    int margin_left;
    int margin_right;
    int char_width;
    bool needs_carriage_return;
} typewriter_t;


/*
 * Application state
 */
struct app_s {
    pxl_input_t in_prev;
    pxl_input_t in_curr;

    typewriter_t tw;
    keyboard_layout_t layout;
} app;

static inline bool
is_pressed(pxl_input_code_t code) {
    return pxl_input_state(&app.in_curr, code) == 1;
}

static inline bool
was_pressed(pxl_input_code_t code) {
    return (pxl_input_state(&app.in_prev, code) == 0) && (pxl_input_state(&app.in_curr, code) == 1);
}

static void
init_typewriter(void) {
    memset(&app.tw, 0, sizeof(app.tw));
    app.tw.length = 0;
    app.tw.cursor_pos = 0;
    app.tw.scroll_line = 0;
    app.tw.cursor_visible = true;
    app.tw.cursor_timer = 0.0;

    app.tw.margin_left = PAGE_MARGIN_LEFT;
    app.tw.margin_right = PAGE_MARGIN_RIGHT;
    app.tw.char_width = 8;
    app.tw.needs_carriage_return = false;
}

static void
draw_text_with_cursor(pxl_canvas_t *cnv, double now) {
    pxl_text_ctx_t w;
    pxl_text_ctx_init(&w, cnv, &pxl_font_typewriter);

    int visible_lines = (H - MARGIN_Y * 2) / pxl_font_typewriter.leading;
    int char_h = pxl_font_typewriter.glyph_height;

    int line_count = 0;
    int line_char_start = 0;
    int cursor_line = 0;
    int cursor_x = app.tw.margin_left;
    bool cursor_on_visible_line = false;

    for (int i = 0; i < app.tw.cursor_pos && i < app.tw.length; i++) {
        if (app.tw.buffer[i] == '\n') {
            cursor_line++;
        }
    }

    for (int i = 0; i <= app.tw.length; i++) {
        if (app.tw.needs_carriage_return && line_count == cursor_line) {
            pxl_text_set_cursor(&w, app.tw.margin_left, w.y);
            app.tw.needs_carriage_return = false;
        }

        if (app.tw.buffer[i] == '\n' || app.tw.buffer[i] == '\0') {
            int line_end = (app.tw.buffer[i] == '\n') ? i : app.tw.length;

            if (line_count >= app.tw.scroll_line && line_count < app.tw.scroll_line + visible_lines) {
                int y_pos = MARGIN_Y + (line_count - app.tw.scroll_line) * pxl_font_typewriter.leading;
                pxl_text_set_cursor(&w, app.tw.margin_left, y_pos);

                for (int j = line_char_start; j < line_end; j++) {
                    if (now >= app.tw.char_appear_time[j]) {
                        if (line_count == cursor_line && j == app.tw.cursor_pos) {
                            cursor_x = w.x;
                            cursor_on_visible_line = true;
                        }

                        pxl_canvas_set_color(cnv, TEXT_COLOR);
                        pxl_draw_rune(&w, app.tw.buffer[j]);
                    }
                }

                if (line_count == cursor_line && app.tw.cursor_pos == line_end) {
                    cursor_x = w.x;
                    cursor_on_visible_line = true;
                }
            }

            line_count++;
            line_char_start = i + 1;
        }
    }

    if (app.tw.cursor_visible && cursor_on_visible_line) {
        int visible_line = cursor_line - app.tw.scroll_line;
        int cursor_y = MARGIN_Y + visible_line * pxl_font_typewriter.leading;

        pxl_canvas_set_color(cnv, CURSOR_COLOR);
        pxl_fill_rect(cnv, cursor_x, cursor_y, 2, char_h);
    }
}

static void
render_typewriter(pxl_canvas_t *cnv, double now) {
    pxl_canvas_set_color(cnv, BG_COLOR);
    pxl_canvas_clear(cnv);

    draw_text_with_cursor(cnv, now);
}

static void
delete_char(void) {
    if (app.tw.cursor_pos > 0 && app.tw.length > 0) {
        memmove(&app.tw.buffer[app.tw.cursor_pos - 1],
                &app.tw.buffer[app.tw.cursor_pos],
                (app.tw.length - app.tw.cursor_pos) * sizeof(uint32_t));
        memmove(&app.tw.char_appear_time[app.tw.cursor_pos - 1],
                &app.tw.char_appear_time[app.tw.cursor_pos],
                (app.tw.length - app.tw.cursor_pos) * sizeof(double));
        app.tw.length--;
        app.tw.cursor_pos--;
        app.tw.buffer[app.tw.length] = 0;
        app.tw.char_appear_time[app.tw.length] = 0;
    }
}

static void
carriage_return(void) {
    int line_start = app.tw.cursor_pos;
    while (line_start > 0 && app.tw.buffer[line_start - 1] != '\n') {
        line_start--;
    }
    app.tw.cursor_pos = line_start;
    app.tw.needs_carriage_return = true;
}

static void
move_cursor_left(void) {
    if (app.tw.cursor_pos > 0) {
        app.tw.cursor_pos--;
    }
}

static void
move_cursor_right(void) {
    if (app.tw.cursor_pos < app.tw.length) {
        app.tw.cursor_pos++;
    }
}

static void
insert_char(int c, double now) {
    if (app.tw.length >= MAX_TEXT_LEN - 1) {
        return;
    }

    int chars_from_start = 0;
    int line_start_pos = app.tw.cursor_pos;
    while (line_start_pos > 0 && app.tw.buffer[line_start_pos - 1] != '\n') {
        line_start_pos--;
        chars_from_start++;
    }

    if (chars_from_start + 1 > PAGE_WIDTH_CHARS) {
        if (app.tw.length >= MAX_TEXT_LEN - 1) {
            return;
        }

        if (app.tw.cursor_pos < app.tw.length) {
            memmove(&app.tw.buffer[app.tw.cursor_pos + 1],
                    &app.tw.buffer[app.tw.cursor_pos],
                    (app.tw.length - app.tw.cursor_pos) * sizeof(uint32_t));
            memmove(&app.tw.char_appear_time[app.tw.cursor_pos + 1],
                    &app.tw.char_appear_time[app.tw.cursor_pos],
                    (app.tw.length - app.tw.cursor_pos) * sizeof(double));
        }

        app.tw.buffer[app.tw.cursor_pos] = '\n';
        app.tw.char_appear_time[app.tw.cursor_pos] = now;
        app.tw.length++;
        app.tw.cursor_pos++;
        app.tw.buffer[app.tw.length] = 0;
        app.tw.char_appear_time[app.tw.length] = 0;

        chars_from_start = 0;
    }

    if (app.tw.cursor_pos < app.tw.length) {
        memmove(&app.tw.buffer[app.tw.cursor_pos + 1],
                &app.tw.buffer[app.tw.cursor_pos],
                (app.tw.length - app.tw.cursor_pos) * sizeof(uint32_t));
        memmove(&app.tw.char_appear_time[app.tw.cursor_pos + 1],
                &app.tw.char_appear_time[app.tw.cursor_pos],
                (app.tw.length - app.tw.cursor_pos) * sizeof(double));
    }

    app.tw.buffer[app.tw.cursor_pos] = (uint32_t)c;
    app.tw.char_appear_time[app.tw.cursor_pos] = now + (app.tw.length * CHAR_APPPEAR_DELAY);
    app.tw.length++;
    app.tw.cursor_pos++;
    app.tw.buffer[app.tw.length] = 0;
    app.tw.char_appear_time[app.tw.length] = 0;
}

static void
log_fps(double now) {
    static double t0 = 0;
    static int n = 0;
    
    if (t0 == 0) {
        t0 = now;
        return;
    }
    n++;
    if (now - t0 >= 1.0) {
        int current_fps = (int)((float)n / (float)(now - t0));
        printf("FPS: %d | Text length: %d characters\r", current_fps, app.tw.length);
        fflush(stdout);
        n = 0;
        t0 = now;
    }
}


/*
 *  Keyboard handling
 */

static void
handle_input_common(void) {
    if (was_pressed(PXL_KEYB_BACKSPACE)) {
        delete_char();
    }

    if (was_pressed(PXL_KEYB_DELETE)) {
        if (app.tw.cursor_pos < app.tw.length) {
            memmove(&app.tw.buffer[app.tw.cursor_pos],
                    &app.tw.buffer[app.tw.cursor_pos + 0],
                    app.tw.length - app.tw.cursor_pos);
            app.tw.length--;
            app.tw.buffer[app.tw.length] = '\0';
        }
    }

    if (was_pressed(PXL_KEYB_LEFT)) {
        move_cursor_left();
    }

    if (was_pressed(PXL_KEYB_RIGHT)) {
        move_cursor_right();
    }

    if (was_pressed(PXL_KEYB_HOME)) {
        app.tw.cursor_pos = -1;
    }

    if (was_pressed(PXL_KEYB_END)) {
        app.tw.cursor_pos = app.tw.length;
    }
}

static void
handle_input_qwerty(double now) {
    handle_input_common();

    if (was_pressed(PXL_KEYB_SPACE))        { insert_char(' ', now); }
    if (was_pressed(PXL_KEYB_APOSTROPHE))   { insert_char('\'', now); }
    if (was_pressed(PXL_KEYB_COMMA))        { insert_char(',', now); }
    if (was_pressed(PXL_KEYB_MINUS))        { insert_char('-', now); }
    if (was_pressed(PXL_KEYB_PERIOD))       { insert_char('.', now); }
    if (was_pressed(PXL_KEYB_SLASH))        { insert_char('/', now); }
    if (was_pressed(PXL_KEYB_SEMICOLON))    { insert_char(';', now); }
    if (was_pressed(PXL_KEYB_EQUAL))         { insert_char('=', now); }
    if (was_pressed(PXL_KEYB_LEFT_BRACKET)) { insert_char('[', now); }
    if (was_pressed(PXL_KEYB_BACKSLASH))    { insert_char('\\', now); }
    if (was_pressed(PXL_KEYB_RIGHT_BRACKET)){ insert_char(']', now); }
    if (was_pressed(PXL_KEYB_GRAVE_ACCENT)) { insert_char('`', now); }

    if (was_pressed(PXL_KEYB_0)) { insert_char('0', now); }
    if (was_pressed(PXL_KEYB_1)) { insert_char('1', now); }
    if (was_pressed(PXL_KEYB_2)) { insert_char('2', now); }
    if (was_pressed(PXL_KEYB_3)) { insert_char('3', now); }
    if (was_pressed(PXL_KEYB_4)) { insert_char('4', now); }
    if (was_pressed(PXL_KEYB_5)) { insert_char('5', now); }
    if (was_pressed(PXL_KEYB_6)) { insert_char('6', now); }
    if (was_pressed(PXL_KEYB_7)) { insert_char('7', now); }
    if (was_pressed(PXL_KEYB_8)) { insert_char('8', now); }
    if (was_pressed(PXL_KEYB_9)) { insert_char('9', now); }

    bool shift_pressed = is_pressed(PXL_KEYB_LSHIFT) || is_pressed(PXL_KEYB_RSHIFT);
    if (was_pressed(PXL_KEYB_A)) { insert_char(shift_pressed ? 'A' : 'a', now); }
    if (was_pressed(PXL_KEYB_B)) { insert_char(shift_pressed ? 'B' : 'b', now); }
    if (was_pressed(PXL_KEYB_C)) { insert_char(shift_pressed ? 'C' : 'c', now); }
    if (was_pressed(PXL_KEYB_D)) { insert_char(shift_pressed ? 'D' : 'd', now); }
    if (was_pressed(PXL_KEYB_E)) { insert_char(shift_pressed ? 'E' : 'e', now); }
    if (was_pressed(PXL_KEYB_F)) { insert_char(shift_pressed ? 'F' : 'f', now); }
    if (was_pressed(PXL_KEYB_G)) { insert_char(shift_pressed ? 'G' : 'g', now); }
    if (was_pressed(PXL_KEYB_H)) { insert_char(shift_pressed ? 'H' : 'h', now); }
    if (was_pressed(PXL_KEYB_I)) { insert_char(shift_pressed ? 'I' : 'i', now); }
    if (was_pressed(PXL_KEYB_J)) { insert_char(shift_pressed ? 'J' : 'j', now); }
    if (was_pressed(PXL_KEYB_K)) { insert_char(shift_pressed ? 'K' : 'k', now); }
    if (was_pressed(PXL_KEYB_L)) { insert_char(shift_pressed ? 'L' : 'l', now); }
    if (was_pressed(PXL_KEYB_M)) { insert_char(shift_pressed ? 'M' : 'm', now); }
    if (was_pressed(PXL_KEYB_N)) { insert_char(shift_pressed ? 'N' : 'n', now); }
    if (was_pressed(PXL_KEYB_O)) { insert_char(shift_pressed ? 'O' : 'o', now); }
    if (was_pressed(PXL_KEYB_P)) { insert_char(shift_pressed ? 'P' : 'p', now); }
    if (was_pressed(PXL_KEYB_Q)) { insert_char(shift_pressed ? 'Q' : 'q', now); }
    if (was_pressed(PXL_KEYB_R)) { insert_char(shift_pressed ? 'R' : 'r', now); }
    if (was_pressed(PXL_KEYB_S)) { insert_char(shift_pressed ? 'S' : 's', now); }
    if (was_pressed(PXL_KEYB_T)) { insert_char(shift_pressed ? 'T' : 't', now); }
    if (was_pressed(PXL_KEYB_U)) { insert_char(shift_pressed ? 'U' : 'u', now); }
    if (was_pressed(PXL_KEYB_V)) { insert_char(shift_pressed ? 'V' : 'v', now); }
    if (was_pressed(PXL_KEYB_W)) { insert_char(shift_pressed ? 'W' : 'w', now); }
    if (was_pressed(PXL_KEYB_X)) { insert_char(shift_pressed ? 'X' : 'x', now); }
    if (was_pressed(PXL_KEYB_Y)) { insert_char(shift_pressed ? 'Y' : 'y', now); }
    if (was_pressed(PXL_KEYB_Z)) { insert_char(shift_pressed ? 'Z' : 'z', now); }

    if (was_pressed(PXL_KEYB_ENTER)) {
        insert_char('\n', now);
    }

    if (was_pressed(PXL_KEYB_R)) {
        carriage_return();
    }
}

static void
handle_input_azerty(double now) {
    handle_input_common();

    bool shift_pressed = is_pressed(PXL_KEYB_LSHIFT) || is_pressed(PXL_KEYB_RSHIFT);
    bool altgr_pressed = is_pressed(PXL_KEYB_RALT);

    if (was_pressed(PXL_KEYB_ENTER)) {
        insert_char('\n', now);
        return;
    }

    if (was_pressed(PXL_KEYB_R)) {
        carriage_return();
        return;
    }

    if (altgr_pressed) {
        if (was_pressed(PXL_KEYB_E))     { insert_char(0x20AB, now); }
        if (was_pressed(PXL_KEYB_7))     { insert_char('[', now); }
        if (was_pressed(PXL_KEYB_MINUS)) { insert_char(']', now); }
        return;
    }

    if (shift_pressed) {
        if (was_pressed(PXL_KEYB_2))        { insert_char(0x00C9, now); }
        if (was_pressed(PXL_KEYB_7))        { insert_char(0x00C8, now); }
        if (was_pressed(PXL_KEYB_9))        { insert_char(0x00C7, now); }
        if (was_pressed(PXL_KEYB_0))        { insert_char(0x00C0, now); }
        if (was_pressed(PXL_KEYB_EQUAL))    { insert_char(0x00D8, now); }
        if (was_pressed(PXL_KEYB_3))        { insert_char('#', now); }
        if (was_pressed(PXL_KEYB_4))        { insert_char('{', now); }
        if (was_pressed(PXL_KEYB_5))        { insert_char('[', now); }
        if (was_pressed(PXL_KEYB_6))        { insert_char('|', now); }
        if (was_pressed(PXL_KEYB_8))        { insert_char('\\', now); }
        if (was_pressed(PXL_KEYB_APOSTROPHE)) { insert_char('~', now); }
        if (was_pressed(PXL_KEYB_EQUAL))    { insert_char('+', now); }
    } else {
        if (was_pressed(PXL_KEYB_2))        { insert_char(0x00E9, now); }
        if (was_pressed(PXL_KEYB_7))        { insert_char(0x00E8, now); }
        if (was_pressed(PXL_KEYB_9))        { insert_char(0x00E7, now); }
        if (was_pressed(PXL_KEYB_0))        { insert_char(0x00E0, now); }
        if (was_pressed(PXL_KEYB_EQUAL))    { insert_char(0x00F8, now); }
        if (was_pressed(PXL_KEYB_SPACE))       { insert_char(' ', now); }
        if (was_pressed(PXL_KEYB_APOSTROPHE))  { insert_char(0x00B1, now); }
        if (was_pressed(PXL_KEYB_3))         { insert_char('"', now); }
        if (was_pressed(PXL_KEYB_4))         { insert_char('\'', now); }
        if (was_pressed(PXL_KEYB_5))         { insert_char('(', now); }
        if (was_pressed(PXL_KEYB_6))         { insert_char('-', now); }
        if (was_pressed(PXL_KEYB_8))         { insert_char('_', now); }
        if (was_pressed(PXL_KEYB_BACKSLASH)) { insert_char(0x00AF, now); }
    }

    if (shift_pressed) {
        if (was_pressed(PXL_KEYB_Q)) { insert_char('A', now); }
        if (was_pressed(PXL_KEYB_A)) { insert_char('Q', now); }
        if (was_pressed(PXL_KEYB_W)) { insert_char('Z', now); }
        if (was_pressed(PXL_KEYB_Z)) { insert_char('W', now); }
        if (was_pressed(PXL_KEYB_B)) { insert_char('B', now); }
        if (was_pressed(PXL_KEYB_C)) { insert_char('C', now); }
        if (was_pressed(PXL_KEYB_D)) { insert_char('D', now); }
        if (was_pressed(PXL_KEYB_E)) { insert_char('E', now); }
        if (was_pressed(PXL_KEYB_F)) { insert_char('F', now); }
        if (was_pressed(PXL_KEYB_G)) { insert_char('G', now); }
        if (was_pressed(PXL_KEYB_H)) { insert_char('H', now); }
        if (was_pressed(PXL_KEYB_I)) { insert_char('I', now); }
        if (was_pressed(PXL_KEYB_J)) { insert_char('J', now); }
        if (was_pressed(PXL_KEYB_K)) { insert_char('K', now); }
        if (was_pressed(PXL_KEYB_L)) { insert_char('L', now); }
        if (was_pressed(PXL_KEYB_COMMA)) { insert_char('M', now); }
        if (was_pressed(PXL_KEYB_N)) { insert_char('N', now); }
        if (was_pressed(PXL_KEYB_O)) { insert_char('O', now); }
        if (was_pressed(PXL_KEYB_P)) { insert_char('P', now); }
        if (was_pressed(PXL_KEYB_R)) { insert_char('R', now); }
        if (was_pressed(PXL_KEYB_S)) { insert_char('S', now); }
        if (was_pressed(PXL_KEYB_T)) { insert_char('T', now); }
        if (was_pressed(PXL_KEYB_U)) { insert_char('U', now); }
        if (was_pressed(PXL_KEYB_V)) { insert_char('V', now); }
        if (was_pressed(PXL_KEYB_X)) { insert_char('X', now); }
        if (was_pressed(PXL_KEYB_Y)) { insert_char('Y', now); }
    } else {
        if (was_pressed(PXL_KEYB_Q)) { insert_char('a', now); }
        if (was_pressed(PXL_KEYB_A)) { insert_char('q', now); }
        if (was_pressed(PXL_KEYB_W)) { insert_char('z', now); }
        if (was_pressed(PXL_KEYB_Z)) { insert_char('w', now); }
        if (was_pressed(PXL_KEYB_B)) { insert_char('b', now); }
        if (was_pressed(PXL_KEYB_C)) { insert_char('c', now); }
        if (was_pressed(PXL_KEYB_D)) { insert_char('d', now); }
        if (was_pressed(PXL_KEYB_E)) { insert_char('e', now); }
        if (was_pressed(PXL_KEYB_F)) { insert_char('f', now); }
        if (was_pressed(PXL_KEYB_G)) { insert_char('g', now); }
        if (was_pressed(PXL_KEYB_H)) { insert_char('h', now); }
        if (was_pressed(PXL_KEYB_I)) { insert_char('i', now); }
        if (was_pressed(PXL_KEYB_J)) { insert_char('j', now); }
        if (was_pressed(PXL_KEYB_K)) { insert_char('k', now); }
        if (was_pressed(PXL_KEYB_L)) { insert_char('l', now); }
        if (was_pressed(PXL_KEYB_COMMA)) { insert_char('m', now); }
        if (was_pressed(PXL_KEYB_N)) { insert_char('n', now); }
        if (was_pressed(PXL_KEYB_O)) { insert_char('o', now); }
        if (was_pressed(PXL_KEYB_P)) { insert_char('p', now); }
        if (was_pressed(PXL_KEYB_R)) { insert_char('r', now); }
        if (was_pressed(PXL_KEYB_S)) { insert_char('s', now); }
        if (was_pressed(PXL_KEYB_T)) { insert_char('t', now); }
        if (was_pressed(PXL_KEYB_U)) { insert_char('u', now); }
        if (was_pressed(PXL_KEYB_V)) { insert_char('v', now); }
        if (was_pressed(PXL_KEYB_X)) { insert_char('x', now); }
        if (was_pressed(PXL_KEYB_Y)) { insert_char('y', now); }
        if (was_pressed(PXL_KEYB_SEMICOLON)) { insert_char(';', now); }
        if (was_pressed(PXL_KEYB_SLASH)) { insert_char('/', now); }
        if (was_pressed(PXL_KEYB_PERIOD)) { insert_char('.', now); }
        if (was_pressed(PXL_KEYB_LEFT_BRACKET)) { insert_char('^', now); }
        if (was_pressed(PXL_KEYB_RIGHT_BRACKET)) { insert_char('$', now); }
        if (was_pressed(PXL_KEYB_GRAVE_ACCENT)) { insert_char('`', now); }
    }
}

static void
handle_input(double now) {
    if (app.layout == KEYBOARD_AZERTY) {
        handle_input_azerty(now);
    } else {
        handle_input_qwerty(now);
    }
}

/*
 * Main
 */

int
main(void) {
    if (pxl_backend_init("PXL Typewriter", W, H, false) != PXL_SUCCESS)
        return 1;

    printf("Typewriter demo. Type text, use arrows to navigate, Backspace/Delete to edit.\n");
    printf("F1=toggle AZERTY/QWERTY, R=carriage return, ESC=quit\n");

    init_typewriter();
    app.layout = KEYBOARD_QWERTY;

    while (!is_pressed(PXL_KEYB_ESCAPE) && !is_pressed(PXL_WM_QUIT)) {
        app.in_prev = app.in_curr;
        pxl_backend_poll_events(&app.in_curr);

        if (was_pressed(PXL_KEYB_F1)) {
            app.layout = (app.layout == KEYBOARD_QWERTY) ? KEYBOARD_AZERTY : KEYBOARD_QWERTY;
            printf("Keyboard layout: %s\n", app.layout == KEYBOARD_AZERTY ? "AZERTY" : "QWERTY");
        }

        double now = pxl_backend_get_time();
		handle_input(now);

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);

            render_typewriter(&cnv, now);

			log_fps(now);
            pxl_backend_end_frame();
        }
    }

    printf("Text length: %d characters\n", app.tw.length);
    pxl_backend_deinit();
    return 0;
}
