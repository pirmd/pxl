#include "input.h"
#include "test.h"

static void
test_pxl_input_state(void) {
	pxl_input_t in = {0};

	/* Set a key as down (code 42) */
	in.state[42 / 64] = 1ULL << (42 % 64);
	ASSERT(pxl_input_state(&in, 42) == 1);
	ASSERT(pxl_input_state(&in, 41) == 0);
	ASSERT(pxl_input_state(&in, 43) == 0);

	/* Set a key in the second word (code 63, last code) */
	in.state[63 / 64] = 1ULL << (63 % 64);
	ASSERT(pxl_input_state(&in, 63) == 1);
	ASSERT(pxl_input_state(&in, 62) == 0);
}

static void
test_pxl_input_press_release(void) {
	pxl_input_t in = {0};

	pxl_input_press(&in, PXL_KEYB_A);
	ASSERT(pxl_input_state(&in, PXL_KEYB_A) == 1);
	ASSERT(pxl_input_state(&in, PXL_KEYB_B) == 0);

	pxl_input_release(&in, PXL_KEYB_A);
	ASSERT(pxl_input_state(&in, PXL_KEYB_A) == 0);

	pxl_input_press(&in, PXL_IN_COUNT - 1);
	ASSERT(pxl_input_state(&in, PXL_IN_COUNT - 1) == 1);

	pxl_input_release(&in, PXL_IN_COUNT - 1);
	ASSERT(pxl_input_state(&in, PXL_IN_COUNT - 1) == 0);
}

/* Main */
int
main(void) {
	test_pxl_input_state();
	test_pxl_input_press_release();

	return 0;
}
