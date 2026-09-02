#include <string.h>
#include "test.h"
#include "camera.h"
#include "buf.h"

/* Fixture ----------------------------------------------------------------- */
#define FIXTURE_W 100
#define FIXTURE_H 100
#define FIXTURE_STRIDE 100

static pxl_t g_buf_data[FIXTURE_STRIDE * FIXTURE_H];
static pxl_buf_t g_buf = {
	.data = g_buf_data,
	.width = FIXTURE_W,
	.height = FIXTURE_H,
	.stride = FIXTURE_STRIDE
};

static inline void
fixture_reset(void) {
	memset(g_buf_data, 0x00, sizeof(g_buf_data));
}

/* Tests for pxl_canvas_move_camera ---------------------------------------- */

static void
test_camera_move_from_origin(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_move_camera(&cnv, 10, 20);
	ASSERT(cnv.offset_x == 10 && cnv.offset_y == 20);
}

static void
test_camera_move_negative(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_move_camera(&cnv, -10, -20);
	ASSERT(cnv.offset_x == -10 && cnv.offset_y == -20);
}

static void
test_camera_move_cumulative(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_move_camera(&cnv, 10, 20);
	pxl_canvas_move_camera(&cnv, 5, 10);
	ASSERT(cnv.offset_x == 15 && cnv.offset_y == 30);
}

/* Tests for pxl_canvas_set_camera ---------------------------------------- */

static void
test_camera_set_absolute(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_set_camera(&cnv, 100, 50);
	ASSERT(cnv.offset_x == 100 && cnv.offset_y == 50);
}

static void
test_camera_set_origin(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	pxl_canvas_set_camera(&cnv, 100, 50);
	
	pxl_canvas_set_camera(&cnv, 0, 0);
	ASSERT(cnv.offset_x == 0 && cnv.offset_y == 0);
}

static void
test_camera_set_negative(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_set_camera(&cnv, -10, -20);
	ASSERT(cnv.offset_x == -10 && cnv.offset_y == -20);
}

/* Tests for pxl_canvas_reset_camera --------------------------------------- */

static void
test_camera_reset(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_set_camera(&cnv, 100, 50);
	ASSERT(cnv.offset_x == 100 && cnv.offset_y == 50);
	
	pxl_canvas_reset_camera(&cnv);
	ASSERT(cnv.offset_x == 0 && cnv.offset_y == 0);
}

static void
test_camera_reset_after_move(void) {
	pxl_canvas_t cnv;
	pxl_canvas_init(&cnv, &g_buf);
	
	pxl_canvas_move_camera(&cnv, 10, 20);
	pxl_canvas_move_camera(&cnv, 5, 10);
	ASSERT(cnv.offset_x == 15 && cnv.offset_y == 30);
	
	pxl_canvas_reset_camera(&cnv);
	ASSERT(cnv.offset_x == 0 && cnv.offset_y == 0);
}

/* Main ------------------------------------------------------------------- */

int
main(void) {
	fixture_reset();
	
	// move_camera tests
	test_camera_move_from_origin();
	test_camera_move_negative();
	test_camera_move_cumulative();
	
	// set_camera tests
	test_camera_set_absolute();
	test_camera_set_origin();
	test_camera_set_negative();
	
	// reset_camera tests
	test_camera_reset();
	test_camera_reset_after_move();
	
	return 0;
}
