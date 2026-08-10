#ifndef PXL_BACKEND_H
#define PXL_BACKEND_H

#include <stdbool.h>
#include <stdint.h>
#include "err.h"
#include "buf.h"
#include "input.h"

/*
 * When adding a new backend, ensure that:
 *   . All backends MUST use ARGB8888 pixel format (A: bits 24-31, R: 16-23,
 *     G: 8-15, B: 0-7) to be consistent with PXL's native color format.
 *     Memory layout depends on endianness:
 *       - Little-endian: [B, G, R, A] (byte 0 = B, byte 1 = G, byte 2 = R, byte 3 = A)
 *       - Big-endian:   [A, R, G, B] (byte 0 = A, byte 1 = R, byte 2 = G, byte 3 = B)
 *     Use pxl_argb/pxl_a/pxl_r/pxl_g/pxl_b from color.h to ensure portability.
 *   . All backends must return pixel-aligned stride in out_pb->stride
 *     (i.e., out_pb->stride * sizeof(pxl_t) must be a valid memory offset)
 *     This has to be enforced by checks in backend implementations.
 */

/* Backend initialization flags.
 *
 * Usage:
 *   pxl_backend_init("Window", 800, 600, PXL_BACKEND_CENTERED | PXL_BACKEND_VSYNC);
 *
 * Notes:
 *   - PXL_BACKEND_VSYNC: May be ignored by some backends (e.g., X11).
 *   - PXL_BACKEND_HIDDEN: Useful for testing (no window visible).
 *   - Flags can be combined using bitwise OR (|).
 */
typedef enum {
	PXL_BACKEND_FULLSCREEN = (1 << 0),  /* Fullscreen mode */
	PXL_BACKEND_HIDDEN     = (1 << 1),  /* Hidden window (for headless testing) */
	PXL_BACKEND_VSYNC      = (1 << 2),  /* Enable vertical sync */
	PXL_BACKEND_CENTERED   = (1 << 3),  /* Center window on screen */
} pxl_backend_flags_t;

/* Initialize the backend */
pxl_err_t
pxl_backend_init(const char *title, int w, int h, pxl_backend_flags_t flags);

/* Cleanup the backend */
void
pxl_backend_deinit(void);

/* Begin frame - fill out_pb with drawable memory */
pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb);

/* End frame - present to screen */
void
pxl_backend_end_frame(void);

/* Get time returns monotonically increasing time in seconds since start of
 * backend
 */
double
pxl_backend_get_time(void);

/* Poll events - drains the event queue and updates the provided input state.
 *
 * The pxl_input_t struct passed as argument MUST be zero-initialized before first use
 * (e.g., pxl_input_t in = {0};). Backends only UPDATE its fields:
 *   - key/mouse button states (via state[] bitset)
 *   - mouse_x, mouse_y (absolute window coordinates, or -1 if not in window)
 *   - mouse_wheel_x/y (delta since last poll)
 */
void
pxl_backend_poll_events(pxl_input_t *in);

/* Text input: get typed text as UTF-8 string.
 *
 * This function retrieves raw UTF-8 text from keyboard input events,
 * converted according to the OS keyboard layout (e.g., AZERTY, QWERTY).
 * The returned string is guaranteed to be valid UTF-8.
 *
 * The function uses an internal fixed-size buffer with FIFO behavior:
 * if the buffer is full, oldest characters are discarded to make room for new ones.
 *
 * Note: Text input must be enabled for this to work. It is automatically
 *       enabled when the backend window has focus.
 *
 * Note: Special keys (ENTER, TAB, BACKSPACE, arrows, etc.) do NOT generate text input.
 *       Use pxl_input_state() with PXL_KEYB_* codes to detect these keys.
 * For UTF-8 to Unicode codepoint conversion, use pxl_utf8_decode() from text.h
 * or your own decoder.
 *
 * Example usage:
 *   char utf8_buf[32];
 *   if (pxl_backend_has_typed_text()) {
 *       int len = pxl_backend_get_typed_text(utf8_buf, sizeof(utf8_buf));
 *       if (len > 0) {
 *           uint32_t rune;
 *           int consumed = pxl_utf8_decode(utf8_buf, &rune);
 *           if (consumed > 0) {
 *               // Process Unicode codepoint 'rune'
 *           }
 *       }
 *   }
 *   // Check for special keys separately:
 *   if (pxl_input_state(&input, PXL_KEYB_ENTER)) {
 *       // Handle Enter key
 *   }
 *
 * Returns:
 *   Number of bytes written (excluding null terminator), or 0 if no text available.
 *   The output buffer is always null-terminated if out_text_max_len > 0.
 */
int
pxl_backend_get_typed_text(char *out_text, int out_text_max_len);

#endif /* PXL_BACKEND_H */
