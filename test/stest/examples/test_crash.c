#include "stest.h"
#include <assert.h>

static void
test_crash(void) {
	assert(0);
}

int
main(int argc, char **argv) {
	st_getopts(argc, argv);
	return ST_RUN(ST_T(test_crash));
}
