#include "stest.h"

static void
test_pass(void) {
	ST_CHECK(1 == 1, "1==1");
}

int
main(int argc, char **argv) {
	st_getopts(argc, argv);
	return ST_RUN(ST_T(test_pass));
}
