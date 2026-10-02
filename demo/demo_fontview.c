/*
 * PXL Demo: Font Viewer
 *
 * Shows PXL core features:
 *   - Font loading and rendering (pxl_font_t, pxl_writer_t)
 *   - Canvas-based rendering with scissor regions
 *   - UTF-8/rune support
 *   - Resizable window handling
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "pxl.h"

/* Font families */
#include "font_9x15.h"
#include "font_wqy_13pts.h"

static const pxl_font_t *font_latin[] = {
	&font_9x15_latin
};

static const pxl_font_t *font_wqy_13pts[] = {
	&wqy_13pts_cjk,
	&wqy_13pts_punct,
	&wqy_13pts_punct_fw
};

static const struct {
	const pxl_font_t **families[2];
	size_t sizes[2];
	const char *names[2];
} fonts = {
	.families = { font_latin, font_wqy_13pts },
	.sizes = { 1, 3 },
	.names = { "Latin 9x15", "CJK WQY 13pts" }
};

#define NUM_FONT_FAMILIES (sizeof(fonts.families) / sizeof(fonts.families[0]))

/* Lorem ipsum texts for each font family */
static const char *lorem_texts[] = {
    "Arma virumque cano, Troiae qui primus ab oris\nItaliam, fato profugus, Laviniaque venit\nlitora, multum ille et terris iactatus et alto",
    "明月几时有？把酒问青天。\n不知天上宫阙，今夕是何年。\n我欲乘风归去，又恐琼楼玉宇"
};

/* UI and LAYOUT --------------------------------------------------------------- */
#define W 800
#define H 600

#define W_PADDING 8
#define H_PADDING 16

#define TITLE_H 16
#define SCROLLBAR_W 8
#define TEXT_PREVIEW_H 64 
#define FOOTER_H 16

#define HELP_W     400 
#define HELP_H     300

#define GLYPH_ZOOM_FACTOR 3
#define GRID_CELL_W 16
#define GRID_CELL_H 16

/* Color */
#define WIN_COLOR   0xFFFDF6E3U  /* Solarized Base3 */
#define BG_COLOR    0xFFEEE8D5U  /* Solarized Base2 */
#define FG_COLOR    0xFF657B83U  /* Solarized Base0 */
#define HI_COLOR    0xFF268BD2U  /* Solarized Blue */

/* UI state ----------------------------------------------------------------- */
typedef struct {
	const pxl_font_t **fonts;
	size_t font_count;
	int font_scale;
	bool show_help;
} ui_t;

static void
ui_draw_text(const ui_t *ui, pxl_canvas_t *cnv, const char *txt, pxl_align_t align) {
	assert(cnv && ui && txt);
	
	pxl_rect_t bbox = pxl_canvas_view(cnv);

	pxl_writer_t w;
	pxl_writer_init(&w, ui->fonts, ui->font_count);

	pxl_rect_t bounds = pxl_text_bounds(&w, txt);
	pxl_rect_t aligned = pxl_align_rect(bounds, bbox, align);

	pxl_writer_set_cursor(&w, aligned.x, aligned.y);
	pxl_draw_text(cnv, &w, txt);
}

/* Geometric layout -------------------------------------------------------- */
typedef struct {
	pxl_rect_t root;
	pxl_rect_t title;
	pxl_rect_t grid;
	pxl_rect_t scrollbar;
	pxl_rect_t glyph_zoom;
	pxl_rect_t glyph_info;
	pxl_rect_t text_preview;
	pxl_rect_t footer;
	pxl_rect_t help;
} layout_t;

static layout_t
compute_layout(void) {
	layout_t l = {0};
	pxl_backend_get_window_size(&l.root.w, &l.root.h);

	/* Content area: centered with horizontal padding (5% each side) */
	pxl_rect_t content = pxl_pad_rect(l.root, l.root.w / 20, l.root.h / 20);

	/* Title at top with padding */
	l.title = pxl_split_rect(&content, TITLE_H, PXL_SIDE_TOP);

	(void)pxl_split_rect(&content, H_PADDING, PXL_SIDE_TOP);

	/* Grid area + scrollbar (40% of original height) */
	l.grid = pxl_split_rect(&content, l.root.h * 4 / 10, PXL_SIDE_TOP);
	l.scrollbar = pxl_split_rect(&l.grid, SCROLLBAR_W, PXL_SIDE_RIGHT);

	(void)pxl_split_rect(&content, H_PADDING, PXL_SIDE_TOP);

	/* Glyph zoom + info section */
	pxl_rect_t zoom_total = pxl_split_rect(&content, l.root.h * 15 / 100, PXL_SIDE_TOP);
	l.glyph_zoom = pxl_split_rect(&zoom_total, zoom_total.h, PXL_SIDE_LEFT); /* Force square */
	(void)pxl_split_rect(&zoom_total, W_PADDING, PXL_SIDE_LEFT);
	l.glyph_info = zoom_total;

	(void)pxl_split_rect(&content, H_PADDING, PXL_SIDE_TOP);

	/* Text preview */
	l.text_preview = pxl_split_rect(&content, TEXT_PREVIEW_H, PXL_SIDE_TOP);

	(void)pxl_split_rect(&content, H_PADDING, PXL_SIDE_TOP);

	/* Text footer */
	l.footer = pxl_split_rect(&content, FOOTER_H, PXL_SIDE_TOP);

	/* Help overlay: full content height */
	l.help  = pxl_align_rect((pxl_rect_t){0, 0, HELP_W, HELP_H}, l.root, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);

	return l;
}

/* Font viewer logic -------------------------------------------------------- */

typedef struct {
	const pxl_bitmask_t *bitmask;
	pxl_rect_t bitmask_r;
	uint32_t codepoint;
	int width;
	int height;
	int offset_x;
	int offset_y;
	int advance;
} glyph_t;

typedef struct {
	int cols;               /* Number of columns in grid */
	int rows;               /* Number of rows in grid */
	int family_idx;         /* Current font family index */
	int glyph_idx;          /* Index of selected glyph in current view */

	/* Internal */
	int glyph_per_page;
	int start_idx;
	int end_idx;

	const pxl_font_t **font;
	int font_size;
	const char *font_name;
	int font_glyph_count;
} fontview_t;

static int
fontview_get_glyph(const fontview_t *fv, int idx, glyph_t *out_glyph) {
	assert(out_glyph);
	assert(idx >= 0 && idx < fv->font_glyph_count);

	const pxl_font_t **family_fonts = fonts.families[fv->family_idx];
	int idx_in_font = idx;

	*out_glyph = (glyph_t){0};

	for (int i = 0; i < fv->font_size; i++) {
		const pxl_font_t *font = family_fonts[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);

		if (idx_in_font < font_count) {
			out_glyph->bitmask = &font->bitmask;
			out_glyph->codepoint = (uint32_t)font->rune_start + (uint32_t)idx_in_font;
			out_glyph->width = font->bitmask.width;
			out_glyph->height = font->glyph_height;

			/* Per-glyph metrics if available */
			if (font->glyph_widths) {
				out_glyph->width = font->glyph_widths[idx_in_font];
			}
			if (font->glyph_offsets_x) {
				out_glyph->offset_x = font->glyph_offsets_x[idx_in_font];
			}
			if (font->glyph_offsets_y) {
				out_glyph->offset_y = font->glyph_offsets_y[idx_in_font];
			}
			if (font->glyph_advances) {
				out_glyph->advance = font->glyph_advances[idx_in_font];
			}

			out_glyph->bitmask_r = (pxl_rect_t){
				.y = idx_in_font * out_glyph->height,
				.w = out_glyph->width,
				.h = out_glyph->height
			};

			return 0;
		}
		idx_in_font -= font_count;
		assert(idx_in_font >= 0);
	}

	/* Fallback (should never happen due to assert) */
	return 1;
}

static void
fontview_next_glyph(fontview_t *fv, int idx_inc) {
	int new_idx = fv->glyph_idx + idx_inc;
	if (new_idx >= 0 && new_idx < fv->font_glyph_count) {
		fv->glyph_idx = new_idx;
	}

	fv->start_idx = (fv->glyph_idx / fv->glyph_per_page) * fv->glyph_per_page;

	fv->end_idx = fv->start_idx + fv->glyph_per_page;
	if (fv->end_idx > fv->font_glyph_count) {
		fv->end_idx = fv->font_glyph_count;
	}
}

static void
fontview_next_font(fontview_t *fv) {
	fv->family_idx = (fv->family_idx + 1) % (int)NUM_FONT_FAMILIES;

	fv->font = fonts.families[fv->family_idx];
	fv->font_size = (int)fonts.sizes[fv->family_idx];
	fv->font_name = fonts.names[fv->family_idx];

	fv->font_glyph_count = 0;
	for (int i = 0; i < fv->font_size; i++) {
		const pxl_font_t *font = fv->font[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);
		fv->font_glyph_count += font_count;
	}

	fv->glyph_idx = -1;
	fontview_next_glyph(fv, 1);
}

static void
fontview_init(fontview_t *fv) {
	fv->family_idx = -1;
	fontview_next_font(fv);
}

static void
handle_resize(fontview_t *fv, layout_t layout) {
	fv->cols = layout.grid.w / GRID_CELL_W;
	fv->rows = layout.grid.h / GRID_CELL_H;
	fv->glyph_per_page = fv->rows * fv->cols;



	fv->start_idx = (fv->glyph_idx / fv->glyph_per_page) * fv->glyph_per_page;
	fv->end_idx = fv->start_idx + fv->glyph_per_page;
	if (fv->end_idx > fv->font_glyph_count) {
		fv->end_idx = fv->font_glyph_count;
	}
}

/* Input ------------------------------------------------------------------- */
static void
handle_fontview_input(pxl_app_t *app, fontview_t *fv) {
	/* Navigation */
	if (pxl_app_was_triggered(app, PXL_KEYB_H) || pxl_app_was_triggered(app, PXL_KEYB_LEFT)) {
		fontview_next_glyph(fv, -1);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_L) || pxl_app_was_triggered(app, PXL_KEYB_RIGHT)) {
		fontview_next_glyph(fv, 1);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_J) || pxl_app_was_triggered(app, PXL_KEYB_DOWN)) {
		fontview_next_glyph(fv, fv->cols);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_K) || pxl_app_was_triggered(app, PXL_KEYB_UP)) {
		fontview_next_glyph(fv, -fv->cols);
	}
}

static void
handle_ui_input(pxl_app_t *app, ui_t *ui, fontview_t *fv) {
	if (pxl_app_was_triggered(app, PXL_KEYB_F)) {
		pxl_backend_toggle_fullscreen();
	}

	if (pxl_app_was_triggered(app, PXL_KEYB_N)) {
		fontview_next_font(fv);
	}

	ui->show_help = pxl_app_is_active(app, PXL_KEYB_COMMA);

	handle_fontview_input(app, fv);
}

/* Render ------------------------------------------------------------------ */
static void
render_title(pxl_canvas_t *cnv, const fontview_t *fv, const ui_t *ui) {
	assert(cnv && fv && ui);

	char txt[128];
	snprintf(txt, sizeof(txt), "PXL Font Viewer - %s", fv->font_name);

	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
}

static void
render_grid(pxl_canvas_t *cnv, const fontview_t *fv, const ui_t *ui) {
	assert(cnv && fv && ui);

	pxl_writer_t w;
	pxl_writer_init(&w, fv->font, (size_t)fv->font_size);

	pxl_canvas_set_color(cnv, FG_COLOR);
	for (int i = fv->start_idx; i < fv->end_idx; i++) {
		glyph_t glyph;
		if (fontview_get_glyph(fv, i, &glyph) != 0) continue;

		int col = (i - fv->start_idx) % fv->cols;
		int row = (i - fv->start_idx) / fv->cols;

		pxl_rect_t cell = (pxl_rect_t){
			col * GRID_CELL_W,
			row * GRID_CELL_H,
			GRID_CELL_W,
			GRID_CELL_H
		};

		pxl_rect_t aligned = pxl_align_rect(
			(pxl_rect_t){0, 0, glyph.width, glyph.height},
			cell,
			PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER
		);

		pxl_writer_set_cursor(&w, aligned.x, aligned.y);
		pxl_draw_rune(cnv, &w, glyph.codepoint);
	}

	/* Highlight current glyph */
	if (fv->glyph_idx >= fv->start_idx && fv->glyph_idx < fv->end_idx) {
		int hi_col = (fv->glyph_idx - fv->start_idx) % fv->cols;
		int hi_row = (fv->glyph_idx - fv->start_idx) / fv->cols;
		pxl_canvas_set_color(cnv, HI_COLOR);
		pxl_draw_rect(cnv, hi_col * GRID_CELL_W, hi_row * GRID_CELL_H, GRID_CELL_W, GRID_CELL_H);
	}
}

static void
render_scrollbar(pxl_canvas_t *cnv, const fontview_t *fv) {
	assert(cnv && fv);

	pxl_rect_t bar = pxl_canvas_view(cnv);

	/* Calculate thumb size based on visible portion */
	int scroll_h = fv->font_glyph_count > 0 ?
				(bar.h * fv->glyph_per_page) / fv->font_glyph_count : 0;
	scroll_h = scroll_h < 8 ? 8 : scroll_h;

	/* Calculate thumb position based on start_idx */
	float position_ratio = fv->font_glyph_count > 0 ?
				(float)fv->start_idx / (float)fv->font_glyph_count : 0.0f;
	int thumb_y = bar.y + (int)(position_ratio * (float)(bar.h - scroll_h));

	pxl_canvas_set_color(cnv, FG_COLOR);
	pxl_fill_rect(cnv, bar.x, thumb_y, bar.w, scroll_h);
}

static void
render_glyph_zoom(pxl_canvas_t *cnv, const fontview_t *fv) {
	assert(cnv && fv);

	glyph_t glyph;
	if (fontview_get_glyph(fv, fv->glyph_idx, &glyph) != 0) return;

	pxl_rect_t target = pxl_canvas_view(cnv);
	pxl_rect_t src = (pxl_rect_t){
		0, 0,
		glyph.width * GLYPH_ZOOM_FACTOR,
		glyph.height * GLYPH_ZOOM_FACTOR
	};

	pxl_rect_t aligned = pxl_align_rect(src, target, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);

	pxl_canvas_set_color(cnv, HI_COLOR);
	pxl_draw_bitmask_transformed(cnv, glyph.bitmask, glyph.bitmask_r,
						aligned.x, aligned.y, GLYPH_ZOOM_FACTOR, PXL_FLIP_NONE);
}

static void
outline_rect(pxl_canvas_t *cnv, pxl_rect_t r, int thick) {
	pxl_fill_rect(cnv, r.x, r.y, r.w, thick);
	pxl_fill_rect(cnv, r.x, r.y, thick, r.h);
	pxl_fill_rect(cnv, r.x + r.w - thick, r.y, thick, r.h);
	pxl_fill_rect(cnv, r.x, r.y + r.h - thick, r.w, thick);
}

static void
render_glyph_info(pxl_canvas_t *cnv, const fontview_t *fv, const ui_t *ui) {
	assert(cnv && fv && ui);

	glyph_t glyph;
	if (fontview_get_glyph(fv, fv->glyph_idx, &glyph) != 0) return;

	char txt[256];
	snprintf(txt, sizeof(txt),
		"U+%04X\nW:%d  H:%d\nOff:(%+d,%+d) Adv:%d",
		(unsigned int)glyph.codepoint, glyph.width, glyph.height,
		glyph.offset_x, glyph.offset_y, glyph.advance
	);

	pxl_canvas_set_color(cnv, HI_COLOR);

	pxl_writer_t w;
	pxl_writer_init(&w, ui->fonts, ui->font_count);

	pxl_rect_t bbox = pxl_canvas_view(cnv);
	pxl_rect_t bounds = pxl_text_bounds(&w, txt);
	pxl_rect_t aligned = pxl_align_rect(bounds, bbox, PXL_ALIGN_LEFT|PXL_ALIGN_V_CENTER);

	pxl_writer_set_cursor(&w, aligned.x + W_PADDING, aligned.y);
	pxl_draw_text(cnv, &w, txt);
}

static void
render_text_preview(pxl_canvas_t *cnv, const fontview_t *fv, const ui_t *ui) {
	assert(cnv && fv && ui);

	const char *txt = lorem_texts[fv->family_idx];

	pxl_writer_t w;
	pxl_writer_init(&w, fv->font, (size_t)fv->font_size);

	pxl_rect_t bbox = pxl_canvas_view(cnv);
	pxl_rect_t bounds = pxl_text_bounds(&w, txt);
	pxl_rect_t aligned = pxl_align_rect(bounds, bbox, PXL_ALIGN_V_CENTER);

	pxl_canvas_set_color(cnv, FG_COLOR);
	pxl_writer_set_cursor(&w, aligned.x, aligned.y);
	pxl_draw_text(cnv, &w, txt);
}

static void
render_footer(pxl_canvas_t *cnv, const fontview_t *fv, const ui_t *ui) {
	assert(cnv && fv && ui);

	char txt[256];
	snprintf(txt, sizeof(txt),
		"Family: %d/%zu | %s (%d glyphs)",
		fv->family_idx + 1, NUM_FONT_FAMILIES,
		fv->font_name,
		fv->font_glyph_count
	);

	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
}

static void
render_help(pxl_canvas_t *cnv, const ui_t *ui) {
	assert(cnv && ui);

	const char txt[] =
		"   PXL Font Viewer - Controls:\n"
		"\n"
		"H/J: prev glyph  L/K: next glyph\n"
		"N: next font\n"
		",: help  F: fullscreen\n"
		"ESC: quit";

	pxl_canvas_set_color(cnv, FG_COLOR);
	ui_draw_text(ui, cnv, txt, PXL_ALIGN_H_CENTER|PXL_ALIGN_V_CENTER);
	outline_rect(cnv, pxl_canvas_view(cnv), 2);
}

/* Main -------------------------------------------------------------------- */
int
main(void) {
	pxl_app_cfg_t cfg = {
		.title = "PXL Font Viewer",
		.width = W,
		.height = H,
		.backend_flags = PXL_BACKEND_RESIZABLE,
	};

	pxl_app_t app;
	if (pxl_app_init(&app, &cfg) != PXL_SUCCESS) return 1;

	/* Initialize UI */
	ui_t ui = {
		.fonts = font_latin,
		.font_count = 1,
		.font_scale = 1,
		.show_help = false
	};

	/* Initialize font viewer */
	fontview_t fv;
	fontview_init(&fv);

	/* Initial layout */
	layout_t layout = compute_layout();
	handle_resize(&fv, layout);

	while (pxl_app_advance_wait(&app)) {
		if (pxl_app_was_triggered(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		/* Handle input */
		handle_ui_input(&app, &ui, &fv);

		/* Recompute layout every frame to handle resize */
		layout = compute_layout();
		handle_resize(&fv, layout);

		/* Begin frame */
		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			/* Setup canvases */
			pxl_canvas_t cnv, cnv_title, cnv_grid, cnv_scrollbar, cnv_glyph_zoom, cnv_glyph_info;
			pxl_canvas_t cnv_text_preview, cnv_footer, cnv_help;

			pxl_canvas_init(&cnv, &pb);
			pxl_canvas_init_view(&cnv_title, &pb, layout.title);
			pxl_canvas_init_view(&cnv_grid, &pb, layout.grid);
			pxl_canvas_init_view(&cnv_scrollbar, &pb, layout.scrollbar);
			pxl_canvas_init_view(&cnv_glyph_zoom, &pb, layout.glyph_zoom);
			pxl_canvas_init_view(&cnv_glyph_info, &pb, layout.glyph_info);
			pxl_canvas_init_view(&cnv_text_preview, &pb, layout.text_preview);
			pxl_canvas_init_view(&cnv_footer, &pb, layout.footer);
			pxl_canvas_init_view(&cnv_help, &pb, layout.help);

			/* Clear */
			pxl_canvas_set_color(&cnv, WIN_COLOR);
			pxl_canvas_clear(&cnv);

			/* Render components */
			pxl_canvas_set_color(&cnv_title, BG_COLOR);
			pxl_canvas_clear(&cnv_title);
			render_title(&cnv_title, &fv, &ui);

			pxl_canvas_set_color(&cnv_grid, BG_COLOR);
			pxl_canvas_clear(&cnv_grid);
			render_grid(&cnv_grid, &fv, &ui);

			pxl_canvas_set_color(&cnv_scrollbar, BG_COLOR);
			pxl_canvas_clear(&cnv_scrollbar);
			render_scrollbar(&cnv_scrollbar, &fv);

			pxl_canvas_set_color(&cnv_glyph_zoom, BG_COLOR);
			pxl_canvas_clear(&cnv_glyph_zoom);
			render_glyph_zoom(&cnv_glyph_zoom, &fv);

			pxl_canvas_set_color(&cnv_glyph_info, BG_COLOR);
			pxl_canvas_clear(&cnv_glyph_info);
			render_glyph_info(&cnv_glyph_info, &fv, &ui);

			pxl_canvas_set_color(&cnv_text_preview, BG_COLOR);
			pxl_canvas_clear(&cnv_text_preview);
			render_text_preview(&cnv_text_preview, &fv, &ui);

			pxl_canvas_set_color(&cnv_footer, BG_COLOR);
			pxl_canvas_clear(&cnv_footer);
			render_footer(&cnv_footer, &fv, &ui);

			/* Help overlay */
			if (ui.show_help) {
				pxl_canvas_set_color(&cnv_help, BG_COLOR);
				pxl_canvas_clear(&cnv_help);
				render_help(&cnv_help, &ui);
			}

			(void)pxl_backend_end_frame();
		}
	}

	pxl_app_deinit(&app);
	return 0;
}
