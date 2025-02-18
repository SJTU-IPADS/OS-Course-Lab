#!/bin/bash

# filepath: malloclab/handout/malloc-perf/run_redis_benchmark.sh

SCRIPT_DIR=$(pwd)
REDIS_VERSION="7.0.11"
REDIS_SRC_DIR="$SCRIPT_DIR/redis_src"
REDIS_INSTALL_DIR="$SCRIPT_DIR/redis_install"
INTERCEPTOR_FILE="$SCRIPT_DIR/malloc_interceptor.c"
INTERCEPTOR_SO="$SCRIPT_DIR/malloc_interceptor.so"

# 1. Test Redis with malloc interception
echo "Running Redis with malloc interception..."
LD_PRELOAD=$INTERCEPTOR_SO $REDIS_INSTALL_DIR/bin/redis-server --port 6379 &
REDIS_PID=$!

echo "Redis is running with PID $REDIS_PID"

# 2. Run redis-benchmark to generate memory allocation activity
echo "Running redis-benchmark to generate memory allocation activity..."
$REDIS_INSTALL_DIR/bin/redis-benchmark "$@"

# 3. Stop Redis
echo "Stopping Redis server..."
kill $REDIS_PID
wait $REDIS_PID 2>/dev/null || true

echo "Redis setup and malloc interception completed!"