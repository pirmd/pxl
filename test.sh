#!/bin/sh

for backend in x11 sdl; do
    make clean >/dev/null
    make lint test BACKEND="$backend" >/dev/null || exit 1
done

make clean >/dev/null
