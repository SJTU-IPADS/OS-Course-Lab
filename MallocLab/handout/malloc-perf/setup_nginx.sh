#!/bin/bash

set -e  # Exit on error

# 1. Define variables
NGINX_VERSION="1.25.2"
INSTALL_DIR="$(pwd)/nginx_install"
BUILD_DIR="$(pwd)/nginx_build"
INTERCEPT_LIB="$(pwd)/malloc_intercept.so"

# 2. Install dependencies
echo "Installing dependencies..."
sudo apt-get update -y
sudo apt-get install -y build-essential wget libpcre3 libpcre3-dev zlib1g zlib1g-dev gcc make curl apache2-utils

# 3. Download Nginx source code
echo "Downloading Nginx source code..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
wget "http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz" -O nginx.tar.gz
tar -xzf nginx.tar.gz
cd "nginx-${NGINX_VERSION}"

# 4. Configure and build Nginx
echo "Configuring and building Nginx..."
./configure --prefix="$INSTALL_DIR" --with-ld-opt="-L. -ldl"
make
make install

