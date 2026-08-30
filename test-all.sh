#!/bin/sh

# Use Xvfb if no DISPLAY is set and Xvfb is available
if [ -z "$DISPLAY" ] && command -v Xvfb >/dev/null 2>&1; then
    rm -f /tmp/.X99-lock
    pkill -f "Xvfb :99" 2>/dev/null
    sleep 0.5
    Xvfb :99 -screen 0 1024x768x24 &
    XVFB_PID=$!
    export DISPLAY=:99
    sleep 1
    trap "kill $XVFB_PID 2>/dev/null; rm -f /tmp/.X99-lock" EXIT
fi

echo "Testing with default backend (x11)..." >&2
make clean >/dev/null
make lint test BACKEND=x11 >/dev/null || exit 1

for backend in sdl; do
    echo "Testing backend-specific tests for $backend..." >&2
    make clean >/dev/null
    make BACKEND="$backend" >/dev/null || exit 1
    make -C test test_backend >/dev/null || exit 1
done

echo "Testing tools..." >&2
make -C tool test >/dev/null || exit 1

make clean >/dev/null
