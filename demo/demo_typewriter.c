#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pxl.h"
#include "font_9x15.h"

#define W 800
#define H 600
#define FPS 60.0f

/* Solarized color palette */
#define BG_COLOR     0xFFFDF6E3  /* Solarized Base3    */
#define PAPER_COLOR  0xFFEEE8D5  /* Solarized Base2    */
#define TEXT_COLOR   0xFF657B83  /* Solarized Base00   */
#define CURSOR_COLOR 0xFF268BD2  /* Solarized Blue     */

#define PAPER_W  ((W) / 100 * 80)
#define PAPER_H  ((H) / 100 * 80)

#define MARGIN_X (((W) - PAPER_W) / 2)
#define MARGIN_Y (((H) - PAPER_H) / 2)

#define CHAR_W   9
#define CHAR_H   15
#define LEADING  15

#define PAPER_MARGIN  20

#define PAGE_W   ((PAPER_W - 2 * PAPER_MARGIN) / (CHAR_W + 1))
#define PAGE_H   ((PAPER_H - 2 * PAPER_MARGIN) / (CHAR_H + LEADING))

#define PRINT_DURATION  0.5   /* Duration of printing in s */


/*
 *  Typewriter state
 */
typedef struct {
    uint32_t   paper[PAGE_H][PAGE_W];

    int carriage_i, carriage_j;

	double   printing_dt;
	uint32_t is_printing;
	double   is_printing_acc;
} typewriter_t;

static void
init_typewriter(typewriter_t *tw) {
	for (int j = 0; j < PAGE_H; j++) {
		for (int i = 0; i < PAGE_W; i++) {
			tw->paper[j][i] = ' ';
		}
	}

	tw->carriage_i = 0;
	tw->carriage_j = 0;

	tw->printing_dt = PRINT_DURATION;
	tw->is_printing = 0;
	tw->is_printing_acc = 0;
}

static void
carriage_return(typewriter_t *tw) {
	tw->carriage_i  = 0;
	if (tw->carriage_j < PAGE_H - 1) tw->carriage_j++;

}

static void
carriage_home(typewriter_t *tw) {
	tw->carriage_i = 0;
}

static void
carriage_left(typewriter_t *tw) {
	if (tw->carriage_i > 0) tw->carriage_i--;
}

static void
carriage_right(typewriter_t *tw) {
	if (tw->carriage_i < PAGE_W - 1) tw->carriage_i++;
}

static void
carriage_up(typewriter_t *tw) {
	if (tw->carriage_j > 0) tw->carriage_j--;
}

static void
carriage_down(typewriter_t *tw) {
	if (tw->carriage_j < PAGE_H - 1) tw->carriage_j++;
}

static void
press_rune(typewriter_t *tw, uint32_t c) {
	/* Single-hammer machine: a print already in progress cannot be
	 * pre-empted by a new key. Mirrors a real typewriter, where only one
	 * character can be mid-strike at a time. */
	if (tw->is_printing != 0) return;

	tw->is_printing     = c;
	tw->is_printing_acc = 0;
}

static void
update_typewriter(typewriter_t *tw, float dt, bool key_held) {
	if (tw->is_printing == 0) return;

	if (!key_held) {
		/* Key released before the strike completed: nothing gets printed,
		 * exactly like lifting your finger off a typewriter key too soon. */
		tw->is_printing     = 0;
		tw->is_printing_acc = 0;
		return;
	}

	tw->is_printing_acc += dt;
	if (tw->is_printing_acc >= tw->printing_dt) {
		tw->paper[tw->carriage_j][tw->carriage_i] = tw->is_printing;
		carriage_right(tw);

		tw->is_printing     = 0;
		tw->is_printing_acc = 0;
	}
}

static void
render_typewriter(pxl_canvas_t *cnv, typewriter_t *tw) {
    pxl_canvas_set_color(cnv, BG_COLOR);
    pxl_canvas_clear(cnv);

	/* Draw paper sheet with margin */
	pxl_canvas_set_color(cnv, PAPER_COLOR);
	pxl_fill_rect(cnv, MARGIN_X, MARGIN_Y, PAPER_W, PAPER_H);

	pxl_canvas_set_offset(cnv, MARGIN_X + PAPER_MARGIN, MARGIN_Y + PAPER_MARGIN);
	pxl_canvas_set_scissor(cnv, MARGIN_X, MARGIN_Y, PAPER_W, PAPER_H);

    pxl_writer_t w;
    const pxl_font_t *fonts[] = { &font_9x15_latin };
    pxl_writer_init(&w, fonts, 1);
	pxl_writer_set_cursor(&w, 0, 0);
	pxl_canvas_set_color(cnv, TEXT_COLOR);

	for (int j = 0; j < PAGE_H; ++j) {
		for (int i = 0; i < PAGE_W; ++i) {
			pxl_draw_rune(cnv, &w, tw->paper[j][i]);
		}
		pxl_writer_set_cursor(&w, 0, (j + 1) * (CHAR_H + LEADING));
	}

	int cursor_x = tw->carriage_i * (CHAR_W + 1);
	int cursor_y = tw->carriage_j * (CHAR_H + LEADING);
	pxl_canvas_set_color(cnv, CURSOR_COLOR);
	
	if (tw->is_printing > 0) {
		pxl_fill_rect(cnv, cursor_x, cursor_y, CHAR_W, CHAR_H);

		pxl_writer_set_cursor(&w, cursor_x, cursor_y);
		pxl_canvas_set_color(cnv, PAPER_COLOR);
		pxl_draw_rune(cnv, &w, tw->is_printing);
	} else {
		pxl_draw_rect(cnv, cursor_x, cursor_y, CHAR_W, CHAR_H);
	}
}


/*
 * Application state
 */
struct app_s {
    pxl_input_t in_prev;
    pxl_input_t in_curr;

    pxl_time_stepper_t stepper;
    typewriter_t tw;
} app = {
    .stepper = { .dt = 1.0f / FPS }
};

static inline bool
is_pressed(pxl_input_code_t code) {
    return pxl_input_state(&app.in_curr, code) == 1;
}

static inline bool
was_pressed(pxl_input_code_t code) {
    return (pxl_input_state(&app.in_prev, code) == 0) && (pxl_input_state(&app.in_curr, code) == 1);
}

/* Detects whether any printable key is physically held down, using raw key
 * state rather than the typed-text event stream. Typed-text events fire on
 * the OS's key-repeat schedule, which varies by platform/user settings and
 * cannot reliably signal "key is still held" -- physical key state can. */
static inline bool
any_printable_key_held(void) {
    for (pxl_input_code_t c = PXL_KEYB_SPACE; c <= PXL_KEYB_Z; ++c) {
        if (is_pressed(c)) return true;
    }
    return false;
}

/*
 *  Keyboard handling
 */

static void
handle_input(void) {
    /* Handle special keys for cursor movement */
    if (was_pressed(PXL_KEYB_LEFT)) {
		carriage_left(&app.tw);
    }

    if (was_pressed(PXL_KEYB_RIGHT)) {
		carriage_right(&app.tw);
    }

    if (was_pressed(PXL_KEYB_UP)) {
		carriage_up(&app.tw);
    }

    if (was_pressed(PXL_KEYB_DOWN)) {
		carriage_down(&app.tw);
    }

    if (was_pressed(PXL_KEYB_HOME)) {
		carriage_home(&app.tw);
    }

    if (was_pressed(PXL_KEYB_ENTER)) {
		carriage_return(&app.tw);
    }

    /* Process typed text from backend (UTF-8 or Latin-1) */
    char utf8_buf[32];
    int len = pxl_backend_get_typed_text(utf8_buf, sizeof(utf8_buf));
    if (len > 0) {
        const char *ptr = utf8_buf;
        const char *end = utf8_buf + len;
        
        while (ptr < end) {
            uint32_t rune;
            int consumed = pxl_utf8_decode(ptr, &rune);
            /* Filter out control characters (handled by special keys) */
            if (rune >= ' ' && rune != 127) {
                press_rune(&app.tw, rune);
            }
            ptr += consumed;
        }
    }
}

/*
 * Main
 */

int
main(void) {
    if (pxl_backend_init("PXL Typewriter", W, H, 0) != PXL_SUCCESS)
        return 1;

    printf("Typewriter demo. Type text, use arrows to navigate.\n");
    printf("R=carriage return, Arrows=move cursor, ESC=quit\n");

    /* CHAR_H+LEADING drives the compile-time paper[][] sizing (PAGE_H), so
     * they can't be derived from the font struct directly -- catch any
     * silent desync with the actual font metrics here instead. */
    assert(font_9x15_latin.glyph_height == CHAR_H);
    assert(font_9x15_latin.leading == LEADING);

    init_typewriter(&app.tw);

    pxl_stepper_init(&app.stepper, pxl_backend_get_time());

    while (!is_pressed(PXL_KEYB_ESCAPE) && !is_pressed(PXL_WM_QUIT)) {
        app.in_prev = app.in_curr;
        pxl_backend_poll_events(&app.in_curr);

		handle_input();

        pxl_stepper_sync_time(&app.stepper, pxl_backend_get_time());

		bool key_held = any_printable_key_held();
		while (pxl_stepper_advance(&app.stepper)) {
			update_typewriter(&app.tw, (float)app.stepper.dt, key_held);
		}

        pxl_buf_t pb;
        if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
            pxl_canvas_t cnv;
            pxl_canvas_init(&cnv, &pb);

            render_typewriter(&cnv, &app.tw);

            (void)pxl_backend_end_frame();
        }
    }

	printf("\n");

    pxl_backend_deinit();
    return 0;
}
