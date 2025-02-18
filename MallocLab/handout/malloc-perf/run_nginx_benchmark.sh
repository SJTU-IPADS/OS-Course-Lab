INTERCEPT_LIB="$(pwd)/malloc_interceptor.so"
INSTALL_DIR="$(pwd)/nginx_install"

# 1. Configure environment variable to intercept memory allocation operations
export LD_PRELOAD="$INTERCEPT_LIB"

# 2. Start Nginx
echo "Starting Nginx..."
"$INSTALL_DIR/sbin/nginx" -c "$(pwd)/nginx.conf"

# 3. Test Nginx service
echo "Testing Nginx service..."
curl -I http://localhost:8080

# 4. Run performance test using Apache Benchmark
echo "Running performance test with Apache Benchmark..."
ab -n 1000 -c 50 http://localhost:$NGINX_PORT/

# 5. Stop Nginx
echo "Stopping Nginx..."
"$INSTALL_DIR/sbin/nginx" -s stop

echo "Script completed!"