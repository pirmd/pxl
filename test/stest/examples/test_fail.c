#include "stest.h"

static void
test_fail_check(void) {
	ST_CHECK(0, "expected failure");
}

int
main(int argc, char **argv) {
	st_getopts(argc, argv);
	return ST_RUN(ST_T(test_fail_check));
}
