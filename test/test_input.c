#include <assert.h>
#include <stdbool.h>
#include "input.h"

static void
test_pxl_input_state(void) {
	pxl_input_t in = {0};

	in.state[42 / 64] = 1ULL << (42 % 64);
	assert(pxl_input_state(&in, 42) == true);
	assert(pxl_input_state(&in, 41) == false);
	assert(pxl_input_state(&in, 43) == false);

	in.state[63 / 64] = 1ULL << (63 % 64);
	assert(pxl_input_state(&in, 63) == true);
	assert(pxl_input_state(&in, 62) == false);
}

static void
test_pxl_input_set_clear(void) {
	pxl_input_t in = {0};

	pxl_input_set(&in, PXL_KEYB_A);
	assert(pxl_input_state(&in, PXL_KEYB_A) == true);
	assert(pxl_input_state(&in, PXL_KEYB_B) == false);

	pxl_input_unset(&in, PXL_KEYB_A);
	assert(pxl_input_state(&in, PXL_KEYB_A) == false);

	pxl_input_set(&in, PXL_IN_COUNT - 1);
	assert(pxl_input_state(&in, PXL_IN_COUNT - 1) == true);

	pxl_input_unset(&in, PXL_IN_COUNT - 1);
	assert(pxl_input_state(&in, PXL_IN_COUNT - 1) == false);
}

/* Main */
int
main(void) {
	test_pxl_input_state();
	test_pxl_input_set_clear();

	return 0;
}
