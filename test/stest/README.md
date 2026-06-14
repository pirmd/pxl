# stest - Minimal C Testing Framework

## Features
- Crash detection (SIGSEGV, SIGFPE, SIGABRT, etc.)
- Filter tests by name

## Usage

### Write a test
```c
#include "stest.h"

static void
test_example(void) {
	ST_CHECK(1 == 1, "math works");
}

static void
test_other_example(void) {
	ST_CHECK(2 == 2, "advanced math also works");
}

int
main(void) {
	return ST_RUN(
        ST_T(test_example),
        ST_T(test_other_example),
    );
}
```

## Build, Install & Run
```sh
# Build library
cd stest && make

# Build and run example tests
cd stest/examples && make test
```

### Build Variables
In addition to standard `CC`, `CFLAGS`, `LDFLAGS`, `PREFIX`:
- `DESTDIR`: Installation prefix (default: empty)
- `STESTDIR`: Path to stest directory (default: `..` from examples)

### Installation
```sh
cd stest && make install PREFIX=/usr/local
```

### Un-installation
```sh
cd stest && make uninstall PREFIX=/usr/local
```
