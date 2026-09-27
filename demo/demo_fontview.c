/*
 * PXL Demo: Font Viewer
 *
 * Static font visualization demo showcasing PXL's font system:
 *   - Font loading and rendering (pxl_font_t, pxl_writer_t)
 *   - Bitmask font rendering
 *   - Canvas with scissor regions
 *   - UTF-8/rune support
 *
 * This is a STATIC demo (no physics loop) using pxl_app_advance_wait().
 * For interactive demos with movement/physics, see demo_pong.c.
 *
 * Note: font_9x15.h and font_wqy_13pts.h are generated font headers (see tool/bdf2pxl).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "pxl.h"
#include "demo_helpers.h"

#include "font_9x15.h"
#include "font_wqy_13pts.h"

/* Font families */
static const pxl_font_t *font_family_latin[] = {
	&font_9x15_latin
};

static const pxl_font_t *font_family_wqy_13pts[] = {
	&wqy_13pts_cjk,
	&wqy_13pts_punct,
	&wqy_13pts_punct_fw
};

static const pxl_font_t **font_families[] = {
	font_family_latin,
	font_family_wqy_13pts
};

static const size_t font_family_sizes[] = {
	1,  /* latin */
	3   /* wqy 13pts */
};

static const char *font_family_names[] = {
	"Latin 9x15",
	"CJK WQY 13pts"
};

#define NUM_FONT_FAMILIES (sizeof(font_families) / sizeof(font_families[0]))

#define BG                   0xFFFDF6E3  /* Solarized Base3 */
#define GRID_VIEW_BG         0xFFEEE8D5  /* Solarized Base2 */
#define GRID_VIEW_FG         0xFF657B83  /* Solarized Base0 */
#define GRID_VIEW_SELECT_FG  0xFF268BD2  /* Solarized Blue */
#define SCROLLBAR_BG         GRID_VIEW_BG
#define SCROLLBAR_FG         GRID_VIEW_FG
#define TITLE_BG             GRID_VIEW_BG 
#define TITLE_FG             GRID_VIEW_FG
#define GLYPH_ZOOM_BG        GRID_VIEW_BG
#define GLYPH_ZOOM_FG        GRID_VIEW_SELECT_FG
#define GLYPH_BG             GRID_VIEW_BG
#define GLYPH_FG             GRID_VIEW_SELECT_FG
#define FOOTER_BG            GRID_VIEW_BG
#define FOOTER_FG            GRID_VIEW_FG
#define TEXT_PREVIEW_BG      GRID_VIEW_BG
#define TEXT_PREVIEW_FG      GRID_VIEW_FG

/* Lorem ipsum texts for each font family */
static const char *lorem_texts[] = {
    "Lorem ipsum dolor sit amet consectetur adipiscing elit sed do",
    "中文字体测试文本用于展示字体效果和排版"
};

#define W_PADDING 8
#define H_PADDING 16
#define GRID_VIEW_CELL_W 16
#define GRID_VIEW_CELL_H 16
#define GLYPH_ZOOM_FACTOR 3
#define TEXT_PREVIEW_H    40
#define TITLE_H 16
#define SCROLLBAR_W 8

/* Minimum dimensions for various components */
#define MIN_GRID_COLS 10
#define MIN_GRID_ROWS 3
#define MIN_GLYPH_ZOOM_SIZE 40

/* Layout structure to hold dynamically calculated dimensions */
typedef struct {
    int window_w;
    int window_h;
    
    /* Title area */
    int title_x;
    int title_y;
    int title_w;
    int title_h;
    
    /* Grid view area */
    int grid_view_x;
    int grid_view_y;
    int grid_view_w;
    int grid_view_h;
    int grid_view_cols;
    int grid_view_rows;
    
    /* Scrollbar */
    int scrollbar_x;
    int scrollbar_y;
    int scrollbar_w;
    int scrollbar_h;
    
    /* Grid (grid view + scrollbar) */
    int grid_x;
    int grid_y;
    int grid_w;
    int grid_h;
    
    /* Glyph zoom */
    int glyph_zoom_x;
    int glyph_zoom_y;
    int glyph_zoom_w;
    int glyph_zoom_h;
    
    /* Glyph characteristics */
    int glyph_x;
    int glyph_y;
    int glyph_w;
    int glyph_h;
    
    /* Text preview */
    int text_preview_x;
    int text_preview_y;
    int text_preview_w;
    int text_preview_h;
    
    /* Footer */
    int footer_x;
    int footer_y;
    int footer_w;
    int footer_h;
} layout_t;

/* Calculate all UI element positions and sizes based on window dimensions.
 * Adjusts grid columns/rows to fit available space, shows/hides scrollbar as needed. */
static void
layout_calculate(layout_t *layout, int window_w, int window_h) {
    assert(layout);
    assert(window_w > 0 && window_h > 0);
    
    layout->window_w = window_w;
    layout->window_h = window_h;
    
    /* Calculate grid view dimensions based on available space */
    /* Use 80% of window width for grid, leave margins */
    int grid_max_w = (int)((float)window_w * 0.8f);
    int grid_max_h = (int)((float)window_h * 0.4f);  /* Grid takes ~40% of height */
    
    /* Calculate max columns and rows that fit */
    int max_cols = grid_max_w / GRID_VIEW_CELL_W;
    int max_rows = grid_max_h / GRID_VIEW_CELL_H;
    
    /* Clamp to minimum values */
    layout->grid_view_cols = max_cols < MIN_GRID_COLS ? MIN_GRID_COLS : max_cols;
    layout->grid_view_rows = max_rows < MIN_GRID_ROWS ? MIN_GRID_ROWS : max_rows;
    
    /* Recalculate grid view dimensions */
    layout->grid_view_w = layout->grid_view_cols * GRID_VIEW_CELL_W;
    layout->grid_view_h = layout->grid_view_rows * GRID_VIEW_CELL_H;
    
    /* Center grid horizontally, position vertically with padding */
    layout->grid_view_x = (window_w - layout->grid_view_w) / 2;
    layout->grid_view_y = H_PADDING + TITLE_H + H_PADDING;
    
    /* Scrollbar dimensions */
    layout->scrollbar_w = SCROLLBAR_W;
    layout->scrollbar_h = layout->grid_view_h;
    layout->scrollbar_x = layout->grid_view_x + layout->grid_view_w + W_PADDING;
    layout->scrollbar_y = layout->grid_view_y;
    
    /* Grid area (grid view + scrollbar) */
    layout->grid_x = layout->grid_view_x;
    layout->grid_y = layout->grid_view_y;
    layout->grid_w = layout->grid_view_w + (W_PADDING + layout->scrollbar_w);
    layout->grid_h = layout->grid_view_h;
    
    /* Title area (centered horizontally, above grid) */
    layout->title_x = layout->grid_x;
    layout->title_y = H_PADDING;
    layout->title_w = layout->grid_w;
    layout->title_h = TITLE_H;
    
    /* Glyph zoom area (below grid) */
    layout->glyph_zoom_w = MIN_GLYPH_ZOOM_SIZE;
    layout->glyph_zoom_h = MIN_GLYPH_ZOOM_SIZE;
    layout->glyph_zoom_x = layout->grid_x;
    layout->glyph_zoom_y = layout->grid_view_y + layout->grid_view_h + H_PADDING;
    
    /* Glyph characteristics area (right of glyph zoom) */
    layout->glyph_x = layout->glyph_zoom_x + layout->glyph_zoom_w + W_PADDING;
    layout->glyph_y = layout->glyph_zoom_y;
    layout->glyph_w = layout->grid_w - layout->glyph_zoom_w - W_PADDING;
    layout->glyph_h = layout->glyph_zoom_h;
    
    /* Text preview area (below glyph zoom and characteristics) */
    layout->text_preview_x = layout->grid_x;
    layout->text_preview_y = layout->glyph_y + layout->glyph_h + H_PADDING;
    layout->text_preview_w = layout->grid_w;
    layout->text_preview_h = TEXT_PREVIEW_H;
    
    /* Footer area (below text preview) */
    layout->footer_x = layout->grid_x;
    layout->footer_y = layout->text_preview_y + layout->text_preview_h + H_PADDING;
    layout->footer_w = layout->grid_w;
    layout->footer_h = TITLE_H;
}


static void
fmt_mem_size(size_t bytes, char *buf, size_t buf_size) {
	if (bytes < 1024) {
		snprintf(buf, buf_size, "%zu B", bytes);
	} else if (bytes < 1024 * 1024) {
		snprintf(buf, buf_size, "%.1f KiB", (double)bytes / 1024.0);
	} else if (bytes < 1024 * 1024 * 1024) {
		snprintf(buf, buf_size, "%.1f MiB", (double)bytes / (1024.0 * 1024.0));
	} else {
		snprintf(buf, buf_size, "%.1f GiB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
	}
}

typedef struct {
	const pxl_bitmask_t *bitmask;
	pxl_rect_t bitmask_r;
	uint32_t codepoint;
	int      width;
	int      height;
	int      offset_x;
	int      offset_y;
	int      advance;
} glyph_t;

typedef struct {
	int cols;               /* Number of columns in grid */
	int rows;               /* Number of rows in grid */
	int family_idx;         /* Current font family index */
	int glyph_idx;          /* Index of selected glyph in current view */

	/* Internal */
	int   glyph_per_page;
	int   start_idx;
	int   end_idx;

	const pxl_font_t **font_family;
	int               font_family_size;
	const char       *font_family_name;
	int               font_family_glyph_count;
	
	/* Dynamic layout */
	layout_t layout;
} font_view_t;

static int
font_view_glyph(const font_view_t *fv, int idx, glyph_t *out_glyph) {
	assert(out_glyph);
	assert(idx >= 0 && idx < fv->font_family_glyph_count);

	const pxl_font_t **family_fonts = font_families[fv->family_idx];
	int idx_in_font = idx;

	*out_glyph = (glyph_t){0};

	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = family_fonts[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);

		if (idx_in_font < font_count) {
			out_glyph->bitmask   = &font->bitmask;
			out_glyph->codepoint = (uint32_t)font->rune_start + (uint32_t)idx_in_font;
			out_glyph->width     = font->bitmask.width;
			out_glyph->height    = font->glyph_height;

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
font_view_next_glyph(font_view_t *fv, int idx_inc) {
	int new_idx = fv->glyph_idx + idx_inc;
	if (new_idx >= 0 && new_idx < fv->font_family_glyph_count) {
		fv->glyph_idx = new_idx;
	}

	fv->start_idx = (fv->glyph_idx / fv->glyph_per_page) * fv->glyph_per_page;
	fv->end_idx = fv->start_idx + fv->glyph_per_page;
	if (fv->end_idx > fv->font_family_glyph_count) fv->end_idx = fv->font_family_glyph_count;
}

static void
font_view_next_font(font_view_t *fv) {
	fv->family_idx = (int)((((size_t)fv->family_idx + 1u) % NUM_FONT_FAMILIES));

	fv->font_family      = font_families[fv->family_idx];
	fv->font_family_size = (int)font_family_sizes[fv->family_idx];
	fv->font_family_name = font_family_names[fv->family_idx];

	fv->font_family_glyph_count = 0;
	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = fv->font_family[i];
		int font_count = (int)(font->rune_end - font->rune_start + 1u);
		fv->font_family_glyph_count += font_count;
	}

	/* Update grid dimensions from layout */
	fv->cols = fv->layout.grid_view_cols;
	fv->rows = fv->layout.grid_view_rows;
	fv->glyph_per_page = fv->cols * fv->rows;

	fv->glyph_idx  = -1;
	font_view_next_glyph(fv, 1);
}

static void
font_view_init(font_view_t *fv) {
	fv->family_idx = -1;
	fv->layout = (layout_t){0};
	font_view_next_font(fv);
}

static size_t
font_view_mem_size(const font_view_t *fv) {
	size_t total_size = 0;
	for (int i = 0; i < fv->font_family_size; i++) {
		const pxl_font_t *font = fv->font_family[i];
		uint32_t glyph_count = font->rune_end - font->rune_start + 1;
		
		/* Bitmask data */
		total_size += (size_t)glyph_count * (size_t)font->glyph_height * (size_t)font->bitmask.stride;
		
		/* Metadata arrays (each is glyph_count elements) */
		if (font->glyph_widths) total_size += glyph_count * sizeof(uint8_t);
		if (font->glyph_advances) total_size += glyph_count * sizeof(uint8_t);
		if (font->glyph_offsets_x) total_size += glyph_count * sizeof(int8_t);
		if (font->glyph_offsets_y) total_size += glyph_count * sizeof(int8_t);
	}
	
	return total_size;
}

static void
handle_resize(font_view_t *fv) {
    int window_w, window_h;
    pxl_backend_get_window_size(&window_w, &window_h);

	/* Recalculate layout with new window dimensions */
	layout_calculate(&fv->layout, window_w, window_h);
	
	/* Update grid dimensions from layout */
	fv->cols = fv->layout.grid_view_cols;
	fv->rows = fv->layout.grid_view_rows;
	fv->glyph_per_page = fv->cols * fv->rows;
	
	/* Recalculate start/end indices to ensure they're valid */
	if (fv->glyph_idx >= fv->glyph_per_page) {
		fv->glyph_idx = fv->glyph_per_page - 1;
		if (fv->glyph_idx < 0) fv->glyph_idx = 0;
	}
	
	fv->start_idx = (fv->glyph_idx / fv->glyph_per_page) * fv->glyph_per_page;
	fv->end_idx = fv->start_idx + fv->glyph_per_page;
	if (fv->end_idx > fv->font_family_glyph_count) {
		fv->end_idx = fv->font_family_glyph_count;
	}
}

static void
handle_input(font_view_t *fv, pxl_app_t *app) {
	/* Handle window resize via event or size change */
	if (pxl_app_was_triggered(app, PXL_WM_RESIZE)) {
		handle_resize(fv);
	}

	if (pxl_app_was_triggered(app, PXL_KEYB_F)) {
		font_view_next_font(fv);
	}

	if (pxl_app_was_triggered(app, PXL_KEYB_H) || pxl_app_is_active(app, PXL_KEYB_LEFT)) {
		font_view_next_glyph(fv, -1);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_J) || pxl_app_is_active(app, PXL_KEYB_DOWN)) {
		font_view_next_glyph(fv, fv->cols);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_K) || pxl_app_is_active(app, PXL_KEYB_UP)) {
		font_view_next_glyph(fv, -fv->cols);
	}
	if (pxl_app_was_triggered(app, PXL_KEYB_L) || pxl_app_is_active(app, PXL_KEYB_RIGHT)) {
		font_view_next_glyph(fv, 1);
	}
}

static void
render_title(pxl_canvas_t *cnv, const font_view_t *fv) {
	char title_text[128];
	snprintf(title_text, sizeof(title_text), "PXL Font Viewer - %s", fv->font_family_name);
	pxl_rect_t title_bounds = pxl_str_bounds(title_text);

	pxl_canvas_set_color(cnv, TITLE_FG);
	pxl_rect_t aligned = pxl_rect_align(title_bounds, (pxl_rect_t){0, 0, fv->layout.title_w, fv->layout.title_h}, PXL_H_CENTER | PXL_V_CENTER);
	int title_x = aligned.x;
	int title_y = aligned.y;
	pxl_draw_str(cnv, title_x, title_y, title_text);
}

static void
render_font_view(pxl_canvas_t *cnv, const font_view_t *fv) {
	pxl_canvas_set_color(cnv, GRID_VIEW_SELECT_FG);
	/* Draw selection rectangle around current glyph (relative to start_idx) */
	int selected_col = (fv->glyph_idx - fv->start_idx) % fv->cols;
	int selected_row = (fv->glyph_idx - fv->start_idx) / fv->cols;
	int selected_x = selected_col * GRID_VIEW_CELL_W;
	int selected_y = selected_row * GRID_VIEW_CELL_H;
	pxl_draw_rect(cnv, selected_x, selected_y, GRID_VIEW_CELL_W, GRID_VIEW_CELL_H);

	pxl_writer_t w;
	pxl_writer_init(&w, fv->font_family, (size_t)fv->font_family_size);
	pxl_canvas_set_color(cnv, GRID_VIEW_FG);

	for (int i = fv->start_idx; i < fv->end_idx; i++) {
		glyph_t glyph;
		if (font_view_glyph(fv, i, &glyph) != 0) continue; /* no glyph found, should not happen */

		/* Position relative to start_idx, accounting for grid wrapping */
		int col = (i - fv->start_idx) % fv->cols;
		int row = (i - fv->start_idx) / fv->cols;
		int cell_x = col * GRID_VIEW_CELL_W;
		int cell_y = row * GRID_VIEW_CELL_H;
		pxl_rect_t aligned = pxl_rect_align((pxl_rect_t){0, 0, glyph.width, glyph.height}, (pxl_rect_t){cell_x, cell_y, GRID_VIEW_CELL_W, GRID_VIEW_CELL_H}, PXL_H_CENTER | PXL_V_CENTER);
		int x = aligned.x;
		int y = aligned.y;

		pxl_writer_set_cursor(&w, x, y);
		pxl_draw_rune(cnv, &w, glyph.codepoint);
	}
}

static void
render_glyph_zoom(pxl_canvas_t *cnv, const font_view_t *fv) {
	glyph_t glyph;
	if (font_view_glyph(fv, fv->glyph_idx, &glyph) != 0) {
		/* no glyph found, should not happen */
		/* TODO: show something as error ? */
		return;
	}

	pxl_rect_t aligned = pxl_rect_align((pxl_rect_t){0, 0, glyph.width * GLYPH_ZOOM_FACTOR, glyph.height * GLYPH_ZOOM_FACTOR}, (pxl_rect_t){0, 0, fv->layout.glyph_zoom_w, fv->layout.glyph_zoom_h}, PXL_H_CENTER | PXL_V_CENTER);
	int zoom_x = aligned.x;
	int zoom_y = aligned.y;

	pxl_canvas_set_color(cnv, GLYPH_ZOOM_FG);
	pxl_draw_bitmask_transformed(cnv, glyph.bitmask, glyph.bitmask_r, zoom_x, zoom_y, GLYPH_ZOOM_FACTOR, PXL_FLIP_NONE);
}

static void
render_glyph_characteristics(pxl_canvas_t *cnv, const font_view_t *fv) {
	glyph_t glyph;
	if (font_view_glyph(fv, fv->glyph_idx, &glyph) != 0) {
		/* no glyph found, should not happen */
		/* TODO: show something as error ? */
		return;
	}

	char text[256];
	snprintf(text, sizeof(text), "U+%04X\nW:%d  H:%d\nOff:(%+d,%+d) Adv:%d",
			(unsigned int)glyph.codepoint, glyph.width, glyph.height,
			glyph.offset_x, glyph.offset_y, glyph.advance);
	
	pxl_canvas_set_color(cnv, GLYPH_FG);

	pxl_rect_t text_bounds = pxl_str_bounds(text);
	int text_x = (fv->layout.glyph_w - text_bounds.w) / 16;
	pxl_rect_t aligned = pxl_rect_align(text_bounds, (pxl_rect_t){0, 0, fv->layout.glyph_w, fv->layout.glyph_h}, PXL_H_LEFT | PXL_V_CENTER);
	int text_y = aligned.y;
	pxl_draw_str(cnv, text_x, text_y, text);
}

static void
render_scrollbar(pxl_canvas_t *cnv, const font_view_t *fv) {
	int page_h = (fv->glyph_per_page * fv->layout.scrollbar_h) / fv->font_family_glyph_count;
	if (page_h < 8) page_h = 8;
	int page_y = (fv->start_idx * (fv->layout.scrollbar_h - page_h)) / (fv->font_family_glyph_count - fv->glyph_per_page);

	pxl_canvas_set_color(cnv, SCROLLBAR_FG);
	/* Position relative to the scrollbar canvas (0,0 is top-left of subview) */
	pxl_fill_rect(cnv, 0, page_y, fv->layout.scrollbar_w, page_h);
}

static void
render_footer(pxl_canvas_t *cnv, const font_view_t *fv) {
	size_t mem_size = font_view_mem_size(fv);

	char mem_size_str[32];
	fmt_mem_size(mem_size, mem_size_str, sizeof(mem_size_str));
	
	char footer_text[256];
	snprintf(footer_text, sizeof(footer_text), "Family: %d/%zu | %s | %s (%d glyphs)",
			fv->family_idx + 1, NUM_FONT_FAMILIES,
		   	fv->font_family_name,
			mem_size_str,
			fv->font_family_glyph_count);

	pxl_rect_t footer_bounds = pxl_str_bounds(footer_text);

	pxl_rect_t aligned = pxl_rect_align(footer_bounds, (pxl_rect_t){0, 0, fv->layout.footer_w, fv->layout.footer_h}, PXL_H_CENTER | PXL_V_CENTER);
	int footer_x = aligned.x;
	int footer_y = aligned.y;

	pxl_canvas_set_color(cnv, FOOTER_FG);
	pxl_draw_str(cnv, footer_x, footer_y, footer_text);
}

static void
render_text_preview(pxl_canvas_t *cnv, const font_view_t *fv) {
	pxl_canvas_set_color(cnv, TEXT_PREVIEW_BG);
	pxl_canvas_clear(cnv);

	const char *text = lorem_texts[fv->family_idx];
	pxl_writer_t w;
	pxl_writer_init(&w, fv->font_family, (size_t)fv->font_family_size);
	pxl_canvas_set_color(cnv, TEXT_PREVIEW_FG);

	pxl_writer_set_cursor(&w, 5, 5);
	pxl_draw_text(cnv, &w, text);
}

static void
render(pxl_canvas_t *cnv, const font_view_t *fv) {
	/* Clear background */
	pxl_canvas_set_color(cnv, BG);
	pxl_canvas_clear(cnv);

	/* Setup viewports for each area using layout dimensions */
	pxl_canvas_t cnv_title = *cnv;
	pxl_canvas_set_offset(&cnv_title, cnv->offset_x + fv->layout.title_x, cnv->offset_y + fv->layout.title_y);
	pxl_canvas_set_scissor(&cnv_title, fv->layout.title_x, fv->layout.title_y, fv->layout.title_w, fv->layout.title_h);

	pxl_canvas_t cnv_grid = *cnv;
	pxl_canvas_set_offset(&cnv_grid, cnv->offset_x + fv->layout.grid_view_x, cnv->offset_y + fv->layout.grid_view_y);
	pxl_canvas_set_scissor(&cnv_grid, fv->layout.grid_view_x, fv->layout.grid_view_y, fv->layout.grid_view_w, fv->layout.grid_view_h);

	pxl_canvas_t cnv_scrollbar = *cnv;
	pxl_canvas_set_offset(&cnv_scrollbar, cnv->offset_x + fv->layout.scrollbar_x, cnv->offset_y + fv->layout.scrollbar_y);
	pxl_canvas_set_scissor(&cnv_scrollbar, fv->layout.scrollbar_x, fv->layout.scrollbar_y, fv->layout.scrollbar_w, fv->layout.scrollbar_h);

	pxl_canvas_t cnv_glyph_zoom = *cnv;
	pxl_canvas_set_offset(&cnv_glyph_zoom, cnv->offset_x + fv->layout.glyph_zoom_x, cnv->offset_y + fv->layout.glyph_zoom_y);
	pxl_canvas_set_scissor(&cnv_glyph_zoom, fv->layout.glyph_zoom_x, fv->layout.glyph_zoom_y, fv->layout.glyph_zoom_w, fv->layout.glyph_zoom_h);

	pxl_canvas_t cnv_glyph = *cnv;
	pxl_canvas_set_offset(&cnv_glyph, cnv->offset_x + fv->layout.glyph_x, cnv->offset_y + fv->layout.glyph_y);
	pxl_canvas_set_scissor(&cnv_glyph, fv->layout.glyph_x, fv->layout.glyph_y, fv->layout.glyph_w, fv->layout.glyph_h);

	pxl_canvas_t cnv_text_preview = *cnv;
	pxl_canvas_set_offset(&cnv_text_preview, cnv->offset_x + fv->layout.text_preview_x, cnv->offset_y + fv->layout.text_preview_y);
	pxl_canvas_set_scissor(&cnv_text_preview, fv->layout.text_preview_x, fv->layout.text_preview_y, fv->layout.text_preview_w, fv->layout.text_preview_h);

	pxl_canvas_t cnv_footer = *cnv;
	pxl_canvas_set_offset(&cnv_footer, cnv->offset_x + fv->layout.footer_x, cnv->offset_y + fv->layout.footer_y);
	pxl_canvas_set_scissor(&cnv_footer, fv->layout.footer_x, fv->layout.footer_y, fv->layout.footer_w, fv->layout.footer_h);

	/* Draw title */
	pxl_canvas_set_color(&cnv_title, TITLE_BG);
	pxl_canvas_clear(&cnv_title);
	render_title(&cnv_title, fv);

	/* Draw glyph grid */
	pxl_canvas_set_color(&cnv_grid, GRID_VIEW_BG);
	pxl_canvas_clear(&cnv_grid);
	render_font_view(&cnv_grid, fv);

	/* Draw scrollbar */
    pxl_canvas_set_color(&cnv_scrollbar, SCROLLBAR_BG);
    pxl_canvas_clear(&cnv_scrollbar);
    render_scrollbar(&cnv_scrollbar, fv);

	/* Draw selected glyph bitmask */
	pxl_canvas_set_color(&cnv_glyph_zoom, GLYPH_ZOOM_BG);
	pxl_canvas_clear(&cnv_glyph_zoom);
	render_glyph_zoom(&cnv_glyph_zoom, fv);

	/* Draw selected glyph characteristics */
	pxl_canvas_set_color(&cnv_glyph, GLYPH_BG);
	pxl_canvas_clear(&cnv_glyph);
	render_glyph_characteristics(&cnv_glyph, fv);

	/* Draw text preview */
	pxl_canvas_set_color(&cnv_text_preview, TEXT_PREVIEW_BG);
	pxl_canvas_clear(&cnv_text_preview);
	render_text_preview(&cnv_text_preview, fv);

	/* Draw footer */
	pxl_canvas_set_color(&cnv_footer, FOOTER_BG);
	pxl_canvas_clear(&cnv_footer);
	render_footer(&cnv_footer, fv);
}

int
main(void) {
	pxl_app_cfg_t cfg = {
		.title = "PXL Font Viewer",
		.width = 720,
		.height = 360,
		.backend_flags = PXL_BACKEND_RESIZABLE,
		/* physics_dt defaults to 0 (no physics stepper) */
	};
	pxl_app_t app;

	if (pxl_app_init(&app, &cfg) != PXL_SUCCESS)
		return 1;

	printf("Font Viewer.\n"
	       "Arrow/HJKL=navigate, F=switch font family.\n"
	       "Resize window to adjust grid. ESC=quit\n");

	font_view_t fv;
	font_view_init(&fv);

	int fps = 0;

	while (pxl_app_advance_wait(&app)) {
		if (pxl_app_was_triggered(&app, PXL_KEYB_ESCAPE)) {
			break;
		}

		handle_input(&fv, &app);
		
		pxl_buf_t pb;
		if (pxl_backend_begin_frame(&pb) == PXL_SUCCESS) {
			pxl_canvas_t cnv;
			pxl_canvas_init(&cnv, &pb);
			render(&cnv, &fv);

			/* Draw FPS in bottom right corner */
			if (fps > 0) {
				char fps_str[16];
				snprintf(fps_str, sizeof(fps_str), "FPS: %d", fps);
				const pxl_font_t *fonts[] = {&font_9x15_latin};
				pxl_writer_t writer;
				pxl_writer_init(&writer, fonts, 1);
				pxl_rect_t fps_bounds = pxl_text_bounds_transformed(&writer, fps_str, 1, PXL_FLIP_NONE);
				pxl_t fg = 0xFFFFFFFFU;
				pxl_canvas_set_color(&cnv, fg);
				/* Align to right/bottom with 10px margin */
				const pxl_rect_t view = pxl_canvas_view(&cnv);
				int w = view.w;
				int h = view.h;
				pxl_rect_t aligned = pxl_rect_align(fps_bounds, (pxl_rect_t){0, 0, w - 10, h - 10}, PXL_H_RIGHT | PXL_V_BOTTOM);
				int fps_x = aligned.x;
				int fps_y = aligned.y;
				pxl_writer_set_cursor(&writer, fps_x, fps_y);
				pxl_draw_text_transformed(&cnv, &writer, fps_str, 1, PXL_FLIP_NONE);
			}

			(void)pxl_backend_end_frame();
		}
		
		 demo_update_fps(app.frame_dt, &fps);
	}

	pxl_app_deinit(&app);
	return 0;
}
