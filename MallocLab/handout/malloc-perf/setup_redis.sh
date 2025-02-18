#!/bin/bash

# filepath: malloclab/handout/malloc-perf/setup_redis.sh

set -e  # Exit on error
echo "Starting Redis setup and malloc interception..."

# 1. Define parameters and directories
SCRIPT_DIR=$(pwd)
REDIS_VERSION="7.0.11"
REDIS_SRC_DIR="$SCRIPT_DIR/redis_src"
REDIS_INSTALL_DIR="$SCRIPT_DIR/redis_install"

# 2. Download Redis source code
echo "Downloading Redis source code..."
mkdir -p $REDIS_SRC_DIR
cd $REDIS_SRC_DIR
if [ ! -f redis-$REDIS_VERSION.tar.gz ]; then
    curl -O http://download.redis.io/releases/redis-$REDIS_VERSION.tar.gz
fi
tar -xzf redis-$REDIS_VERSION.tar.gz
cd redis-$REDIS_VERSION

# 3. Compile Redis using system malloc
echo "Compiling Redis with libc malloc..."
make distclean > /dev/null 2>&1 || true
make MALLOC=libc > /dev/null

# 4. Install Redis
echo "Installing Redis to $REDIS_INSTALL_DIR..."
mkdir -p $REDIS_INSTALL_DIR
make PREFIX=$REDIS_INSTALL_DIR install > /dev/null
echo "Redis installation completed!"
