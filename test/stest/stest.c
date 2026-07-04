#include "stest.h"

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* --- Global runner state ---------------------------------------------- */
static struct {
	int	    status;
	const char *filter;
} g_runner = {
	.status = EXIT_SUCCESS,
	.filter = NULL
};

/* --- Signal names -------------------------------------------------------- */
static const char *
signal_name(int sig) {
	const char *names[] = {
		[SIGHUP]  = "SIGHUP",
		[SIGINT]  = "SIGINT",
		[SIGQUIT] = "SIGQUIT",
		[SIGILL]  = "SIGILL",
		[SIGTRAP] = "SIGTRAP",
		[SIGABRT] = "SIGABRT",
		[SIGBUS]  = "SIGBUS",
		[SIGFPE]  = "SIGFPE",
		[SIGSEGV] = "SIGSEGV",
		[SIGKILL] = "SIGKILL",
		[SIGPIPE] = "SIGPIPE",
		[SIGALRM] = "SIGALRM",
		[SIGTERM] = "SIGTERM",
		[SIGXCPU] = "SIGXCPU",
		[SIGXFSZ] = "SIGXFSZ",
	};
	if (sig >= 0 && (size_t)sig < sizeof(names)/sizeof(names[0])) {
		return names[sig] ? names[sig] : "UNKNOWN";
	}
	return "UNKNOWN";
}

/* --- Core check ---------------------------------------------------------- */
bool
st_check(const st_ctx_t *ctx, int expr, const char *fmt, ...) {
	if (expr) return true;

	if (ctx && ctx->file) fprintf(stderr, "%s:%ld: ", ctx->file, ctx->line);
	if (ctx && ctx->tag)  fprintf(stderr, "%s: ", ctx->tag);
	if (fmt) {
		va_list ap; va_start(ap, fmt);
		vfprintf(stderr, fmt, ap); va_end(ap);
	}
	fputc('\n', stderr);
	fflush(stderr);
	g_runner.status = EXIT_FAILURE;
	return false;
}

/* --- Test runner --------------------------------------------------------- */
static int
run_test(const st_test_t *t) {
	fflush(stdout);
	fflush(stderr);

	pid_t pid = fork();
	if (pid == -1) {
		st_check(&t->ctx, 0, "fork failed");
		return EXIT_FAILURE;
	}

	if (pid == 0) {
		/* Child: run the test */
		g_runner.status = EXIT_SUCCESS;
		t->fn();
		_exit(g_runner.status);
	}

	/* Parent: wait and check result */
	int status;
	if (waitpid(pid, &status, 0) == -1) {
		st_check(&t->ctx, 0, "waitpid failed");
		return EXIT_FAILURE;
	}

	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	if (WIFSIGNALED(status)) {
		int sig = WTERMSIG(status);
		const char *sig_name = signal_name(sig);
		st_check(&t->ctx, 0, "test crashed: %s (signal %d)", sig_name, sig);
		return EXIT_FAILURE;
	}

	st_check(&t->ctx, 0, "test failed: unknown status");
	return EXIT_FAILURE;
}

int
st_run(const st_test_t T[]) {
	int pass = 0, fail = 0, total = 0;

	for (size_t i = 0; T[i].fn != NULL; i++) {
		if (g_runner.filter && !strstr(T[i].name, g_runner.filter)) continue;
		total++;
		if (run_test(&T[i]) == EXIT_SUCCESS) pass++;
		else fail++;
	}

	printf("\n%d test(s): %d passed, %d failed\n", total, pass, fail);
	return (fail > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* --- Argument parsing -------------------------------------------------- */
void
st_getopts(int argc, char **argv) {
	optind = 1; /* Reset for multiple calls */
	int opt;
	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		default:
			fprintf(stderr, "Usage: %s [filter]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	
	if (optind < argc) {
		g_runner.filter = argv[optind];
	}
}
