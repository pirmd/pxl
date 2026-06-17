#include <string.h>
#include "canvas.h"
#include "stest/stest.h"

/* Fixture ----------------------------------------------------------------- */

#define COLOR_WHITE  0xFFFFFFFFU  /* Opaque white */
#define COLOR_RED    0xFFFF0000U  /* Opaque red */
#define COLOR_GREEN  0xFF00FF00U  /* Opaque green */
#define COLOR_BLUE   0xFF0000FFU  /* Opaque blue */
#define COLOR_TRANS  0x00000000U  /* Fully transparent */

static void
pixbuf_zero(pixbuf_t *pb) {
	assert(pb && pb->data);
	memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pix_t));
}

static void
pixbuf_clear(pixbuf_t *pb, pix_t color) {
	assert(pb);
	
	pix_t *dst = pb->data;
	for (int y = 0; y < pb->height; ++y) {
		for (int x = 0; x < pb->width; ++x) {
			dst[x] = color;
		}
		dst += pb->stride;
	}
}

typedef struct {
	pixbuf_t pb;
	canvas_t cnv;
} fixture_t;

static bool
fixture_init(const st_ctx_t *ctx, fixture_t *f, int w, int h) {
	if (!st_check(ctx, pixbuf_init(&f->pb, w, h) == PXL_SUCCESS, "pixbuf_init failed")) {
		return false;
	}

	if (!st_check(ctx, f->pb.data != NULL, "pixbuf data is NULL")) {
		return false;
	}

	canvas_init(&f->cnv, &f->pb);	
	pixbuf_zero(&f->pb);
	
	return true;
}

static void
fixture_deinit(fixture_t *f) {
	pixbuf_deinit(&f->pb);
}


/* Helpers ----------------------------------------------------------------- */

static inline bool
in_scissor(int x, int y, const canvas_t *cnv) {
	const rect_t *s = &cnv->scissor;
	return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

static inline bool
is_on_span(int x, int y, int span_y, int span_x, int span_w) {
	return y == span_y && x >= span_x && x < span_x + span_w;
}

/* Calcule la zone effectivement blittee dans la destination (apres clipping scissor + source).
 *
 * Parametres :
 *   cnv   - Canvas de destination
 *   x,y   - Position de destination (comme dans canvas_blit_rect)
 *   src   - Pixbuf source
 *   w,h   - Dimensions du blit
 *   out   - [OUT] Zone effectivement copiee dans la destination
 *
 * Retourne : true si du contenu a ete blitte, false sinon.
 */
static inline bool
blit_rect_dst_area(const canvas_t *cnv, int x, int y,
                   const pixbuf_t *src, int w, int h, rect_t *out) {
	rect_t dst_r;
	if (!clip_rect((rect_t){x, y, w, h}, cnv->scissor, &dst_r)) {
		return false;
	}

	/* Calculate offset from clipping and apply to source */
	int sx = x + (dst_r.x - x);
	int sy = y + (dst_r.y - y);

	/* Clip source to src bounds */
	rect_t src_r;
	if (!clip_rect((rect_t){sx, sy, dst_r.w, dst_r.h}, pixbuf_bounds(src), &src_r)) {
		return false;
	}

	/* Sync dst_r with src clipping (matching canvas_blit_rect logic) */
	dst_r.x += (src_r.x - sx);
	dst_r.y += (src_r.y - sy);
	dst_r.w = src_r.w;
	dst_r.h = src_r.h;

	*out = dst_r;
	return true;
}

/* Test Lifecycle ---------------------------------------------------------- */

static void
test_canvas_init_basic(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	ST_CHECK(f.cnv.pb == &f.pb, "canvas should reference the pixbuf");
	ST_CHECK(f.cnv.color == COLOR_WHITE, "default color should be white");
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == w && f.cnv.scissor.h == h,
	         "scissor size: want %dx%d, got %dx%d", w, h,
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_set_color(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pix_t want_color = COLOR_GREEN;
	canvas_set_color(&f.cnv, want_color);
	ST_CHECK(f.cnv.color == want_color,
	         "color: want 0x%08X, got 0x%08X",
	         want_color, f.cnv.color);

	fixture_deinit(&f);
}

static void
test_canvas_set_scissor(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, 10, 20, 30, 40);
	ST_CHECK(f.cnv.scissor.x == 10 && f.cnv.scissor.y == 20,
	         "scissor origin: want (10,20), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 30 && f.cnv.scissor.h == 40,
	         "scissor size: want 30x40, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_set_scissor_clipped(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, -10, -10, 200, 200);
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin clamped: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 100 && f.cnv.scissor.h == 100,
	         "scissor size clamped: want 100x100, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_reset_scissor(void) {
	int w = 50, h = 60;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, 10, 10, 20, 20);
	ST_CHECK(f.cnv.scissor.x == 10,
	         "scissor modified: want x=10, got x=%d",
	         f.cnv.scissor.x);

	canvas_reset_scissor(&f.cnv);
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin reset: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 50 && f.cnv.scissor.h == 60,
	         "scissor size reset: want 50x60, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}


/* Test clear -------------------------------------------------------------- */

static void
test_canvas_clear_white(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, 2, 2, 6, 6);
	
	pix_t fg = COLOR_GREEN;
	canvas_set_color(&f.cnv, fg);
	
	canvas_clear(&f.cnv);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &f.cnv);
			pix_t want =  inside_scissor ? fg: 0x00;
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " in scissor": "", want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_clear_full_black(void) {
	int w = 4, h = 4;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pixbuf_clear(&f.pb, COLOR_TRANS);  /* Fill with non-zero then clear with 0 */
	canvas_set_color(&f.cnv, COLOR_TRANS);
	canvas_clear(&f.cnv);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pix_t got = *pixbuf_ptr(&f.pb, x, y);
			ST_CHECK(got == COLOR_TRANS, "pixel (%d,%d) should be transparent (fast path), got 0x%08X", x, y, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_clear_full_white(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pixbuf_clear(&f.pb, COLOR_RED);  /* Fill with non-white pattern */
	canvas_set_color(&f.cnv, COLOR_WHITE);
	canvas_clear(&f.cnv);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pix_t got = *pixbuf_ptr(&f.pb, x, y);
			ST_CHECK(got == COLOR_WHITE, "pixel (%d,%d) should be white (fast path), got 0x%08X", x, y, got);
		}
	}

	fixture_deinit(&f);
}


/* Test fill_span ---------------------------------------------------------- */

static void
test_canvas_fill_span_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}
	
	pix_t fg = COLOR_BLUE;
	canvas_set_color(&f.cnv, fg);

	canvas_fill_span(&f.cnv, 5, 5, 10);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &f.cnv);
			bool on_span = is_on_span(x, y, 5, 5, 10);
			pix_t want = (inside_scissor && on_span) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", on_span ? " [on span]": "", want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_fill_span_clipped(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pix_t fg = COLOR_GREEN;
	canvas_set_color(&f.cnv, fg);

	canvas_fill_span(&f.cnv, 3, 7, 10);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &f.cnv);
			bool    on_span = is_on_span(x, y, 7, 3, 10);
			pix_t want = (inside_scissor && on_span) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", on_span ? " [on span]": "", want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_fill_span_fully_clipped(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pix_t fg = COLOR_GREEN;
	canvas_set_color(&f.cnv, fg);

	canvas_fill_span(&f.cnv, 20, 20, 10);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &f.cnv);
			bool    on_span = is_on_span(x, y, 20, 20, 10);
			pix_t want = (inside_scissor && on_span) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", on_span ? " [on span]": "", want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test blit_rect ---------------------------------------------------------- */


static void
test_canvas_blit_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 10, 10) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t fg = COLOR_RED;
	pixbuf_clear(&src_pb, fg);

	canvas_blit_rect(&dst_f.cnv, 5, 5, &src_pb, 0, 0, 10, 10);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);
			bool    in_blit = in_rect(x, y, (rect_t){5, 5, 10, 10});
			pix_t      want = (inside_scissor && in_blit) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_with_scissor(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 10, 10) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t fg = COLOR_RED;
	pixbuf_clear(&src_pb, fg);

	canvas_set_scissor(&dst_f.cnv, 5, 5, 10, 10);
	canvas_blit_rect(&dst_f.cnv, 0, 0, &src_pb, 0, 0, 20, 20);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);

			rect_t blit_area;
			bool   has_blit = blit_rect_dst_area(&dst_f.cnv, 0, 0, &src_pb, 20, 20, &blit_area);
			bool    in_blit = has_blit && in_rect(x, y, blit_area);
			pix_t      want = (inside_scissor && in_blit) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_clipped_source(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 10, 10) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t fg = COLOR_RED;
	pixbuf_clear(&src_pb, fg);


	canvas_blit_rect(&dst_f.cnv, 5, 5, &src_pb, 5, 5, 20, 20);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);

			rect_t blit_area;
			bool has_blit = blit_rect_dst_area(&dst_f.cnv, 5, 5, &src_pb, 20, 20, &blit_area);
			bool  in_blit = has_blit && in_rect(x, y, blit_area);

			pix_t    want = (inside_scissor && in_blit) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_fully_clipped(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 10, 10) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t fg = COLOR_RED;
	pixbuf_clear(&src_pb, fg);

	/* Blit completely outside scissor */
	canvas_set_scissor(&dst_f.cnv, 5, 5, 10, 10);
	canvas_blit_rect(&dst_f.cnv, 20, 20, &src_pb, 0, 0, 10, 10);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);
			bool    in_blit = in_rect(x, y, (rect_t){20, 20, 10, 10});
			pix_t      want = (inside_scissor && in_blit) ? fg: 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_clipped_left_top(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 20, 20) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t *dst = src_pb.data;
	for (int y = 0; y < src_pb.height; ++y) {
		for (int x = 0; x < src_pb.width; ++x) {
			dst[x] = 0xFF000000U | (x << 16) | (y << 8);
		}
		dst += src_pb.stride;
	}
	
	/* Scissor at (10,10,10,10), blit from (5,5) with w=20,h=20 */
	canvas_set_scissor(&dst_f.cnv, 10, 10, 10, 10);
	canvas_blit_rect(&dst_f.cnv, 5, 5, &src_pb, 0, 0, 20, 20);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);
			bool    in_blit = in_rect(x, y, (rect_t){5, 5, 20, 20});
			pix_t      want = (inside_scissor && in_blit) ? 0xFF000000U | ((x - 5) << 16) | ((y - 5) << 8): 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_clipped_left_only(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 20, 20) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t *dst = src_pb.data;
	for (int y = 0; y < src_pb.height; ++y) {
		for (int x = 0; x < src_pb.width; ++x) {
			dst[x] = 0xFF000000U | (x << 16) | (y << 8);
		}
		dst += src_pb.stride;
	}

	canvas_set_scissor(&dst_f.cnv, 10, 0, 10, 20);
	canvas_blit_rect(&dst_f.cnv, 5, 0, &src_pb, 0, 0, 20, 20);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);
			bool    in_blit = in_rect(x, y, (rect_t){5, 0, 20, 20});
			pix_t      want = (inside_scissor && in_blit) ? 0xFF000000U | ((x - 5) << 16) | ((y - 0) << 8): 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}

static void
test_canvas_blit_rect_clipped_top_only(void) {
	int w = 20, h = 20;
	fixture_t dst_f;
	if (!fixture_init(&ST_HERE, &dst_f, w, h)) {
		return;
	}

	pixbuf_t src_pb;
	if (!ST_CHECK(pixbuf_init(&src_pb, 20, 20) == PXL_SUCCESS, "src pixbuf_init failed")) {
		fixture_deinit(&dst_f);
		return;
	}

	pix_t *dst = src_pb.data;
	for (int y = 0; y < src_pb.height; ++y) {
		for (int x = 0; x < src_pb.width; ++x) {
			dst[x] = 0xFF000000U | (x << 16) | (y << 8);
		}
		dst += src_pb.stride;
	}

	canvas_set_scissor(&dst_f.cnv, 0, 10, 20, 10);
	canvas_blit_rect(&dst_f.cnv, 0, 5, &src_pb, 0, 0, 20, 20);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pix_t  got = *pixbuf_ptr(&dst_f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &dst_f.cnv);
			bool    in_blit = in_rect(x, y, (rect_t){0, 5, 20, 20});
			pix_t      want = (inside_scissor && in_blit) ? 0xFF000000U | ((x - 0) << 16) | ((y - 5) << 8): 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " [in scissor]": "", in_blit ? " [in blit]": "", want, got);
		}
	}

	pixbuf_deinit(&src_pb);
	fixture_deinit(&dst_f);
}


/* Main ------------------------------------------------------------------------- */

int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Life-cycle & accessors */
		ST_T(test_canvas_init_basic),
		ST_T(test_canvas_set_color),
		ST_T(test_canvas_set_scissor),
		ST_T(test_canvas_set_scissor_clipped),
		ST_T(test_canvas_reset_scissor),

		/* canvas_clear */
		ST_T(test_canvas_clear_white),
		ST_T(test_canvas_clear_full_black),
		ST_T(test_canvas_clear_full_white),

		/* canvas_fill_span */
		ST_T(test_canvas_fill_span_basic),
		ST_T(test_canvas_fill_span_clipped),
		ST_T(test_canvas_fill_span_fully_clipped),

		/* canvas_blit_rect */
		ST_T(test_canvas_blit_rect_basic),
		ST_T(test_canvas_blit_rect_with_scissor),
		ST_T(test_canvas_blit_rect_clipped_source),
		ST_T(test_canvas_blit_rect_fully_clipped),
		ST_T(test_canvas_blit_rect_clipped_left_top),
		ST_T(test_canvas_blit_rect_clipped_left_only),
		ST_T(test_canvas_blit_rect_clipped_top_only)
	);
}
