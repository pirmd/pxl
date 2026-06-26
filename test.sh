#!/bin/sh

set -e

run_test() {
    backend=$1
    echo "== TEST CONFIG: BACKEND=$backend"
    
    make clean > /dev/null
    make lint test BACKEND="$backend"
}

run_test "x11"
run_test "sdl"

make clean > /dev/null
