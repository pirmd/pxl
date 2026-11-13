#!/bin/sh
# pxl - full check (make check): lint + unit tests + demo build + both
# backends (x11, sdl) + tools. Picks $MAKE if set, else make (BSD systems),
# else bmake. Needs a DISPLAY for the x11 backend tests.
set -e

[ -z "$DISPLAY" ] && { echo "Error: DISPLAY not set. Please ensure X11 is running or use xhost +local." >&2; exit 1; }

# Pick a make implementation: $MAKE if set, else make (BSD systems), else bmake.
if [ -z "$MAKE" ]; then
	if command -v make >/dev/null 2>&1; then
		MAKE=make
	elif command -v bmake >/dev/null 2>&1; then
		MAKE=bmake
	else
		echo "Error: neither make nor bmake found; set MAKE=/path/to/make" >&2
		exit 1
	fi
fi

echo "Linting..." >&2
"$MAKE" lint >/dev/null

echo "Running tests..." >&2
PXL_BACKEND=x11 "$MAKE" clean >/dev/null && PXL_BACKEND=x11 "$MAKE" test >/dev/null

echo "Testing demo compilation..." >&2
"$MAKE" demo >/dev/null

echo "Testing SDL backend..." >&2
"$MAKE" clean >/dev/null && PXL_BACKEND=sdl "$MAKE" >/dev/null && PXL_BACKEND=sdl "$MAKE" -C test test_backend >/dev/null
PXL_BACKEND=sdl "$MAKE" lint >/dev/null

echo "Testing tools..." >&2
"$MAKE" -C tool test >/dev/null
