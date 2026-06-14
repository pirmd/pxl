#ifndef STEST_H
#define STEST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	const char *file;
	long        line;
	const char *tag;
} st_ctx_t;

typedef struct {
	st_ctx_t    ctx;
	const char *name;
	void      (*fn)(void);
} st_test_t;

int  st_run(const st_test_t T[]);
bool st_check(const st_ctx_t *ctx, int expr, const char *fmt, ...);
void st_getopts(int argc, char **argv);

#define ST_HERE      (st_ctx_t){__FILE__, __LINE__, NULL}
#define ST_T(testfn) (st_test_t){ST_HERE, #testfn, testfn}
#define ST_RUN(...)  st_run((st_test_t[]){ __VA_ARGS__, {0} })

#define ST_CHECK(expr, ...) st_check(&ST_HERE, (expr), __VA_ARGS__)

#define ST_GETOPTS(argc, argv) st_getopts(argc, argv)

#endif /* STEST_H */
