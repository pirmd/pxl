#!/bin/sh
set -e

[ -z "$DISPLAY" ] && { echo "Error: DISPLAY not set. Please ensure X11 is running or use xhost +local." >&2; exit 1; }

echo "Linting..." >&2
make lint >/dev/null

echo "Running tests..." >&2
PXL_BACKEND=x11 make clean >/dev/null && PXL_BACKEND=x11 make test >/dev/null

echo "Testing demo compilation..." >&2
make demo >/dev/null

echo "Testing SDL backend..." >&2
make clean >/dev/null && PXL_BACKEND=sdl make >/dev/null && PXL_BACKEND=sdl make -C test test_backend >/dev/null

echo "Testing tools..." >&2
make -C tool test >/dev/null
