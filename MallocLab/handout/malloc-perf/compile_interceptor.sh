#!/bin/bash

# filepath: malloclab/handout/malloc-perf/compile_interceptor.sh

# Compile the interceptor to a shared library
SCRIPT_DIR=$(pwd)
INTERCEPTOR_FILE="$SCRIPT_DIR/malloc_interceptor.c"
INTERCEPTOR_SO="$SCRIPT_DIR/malloc_interceptor.so"

echo "Compiling interceptor..."
gcc -shared -fPIC -o $INTERCEPTOR_SO $INTERCEPTOR_FILE -ldl

echo "Interceptor compilation completed!"