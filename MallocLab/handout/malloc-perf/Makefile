REDIS_INSTALL_DIR=redis-stable
INTERCEPTOR_FILE=interceptor.c
INTERCEPTOR_SO=interceptor.so
MYSQL_USER="ics"
MYSQL_PASSWORD="ics"
MYSQL_PORT=3306
TEST_DB="test_db"
LOG_FILE_PATH=/home/ics/malloclab/malloc-perf/malloc_log
TRACE_FILE_PATH=/home/ics/malloclab/malloc-perf/trace_file.txt

.PHONY: all setup_redis compile_interceptor run_redis_benchmark run_nginx_benchmark generate_trace clean

all: setup_redis setup_nginx setup_mysql compile_interceptor run_redis_benchmark run_nginx_benchmark run_mysql_benchmark

setup_redis:
	./setup_redis.sh

setup_nginx:
	./setup_nginx.sh

setup_mysql:
	./setup_mysql.sh

compile_interceptor:
	./compile_interceptor.sh

run_redis_benchmark:
	./run_redis_benchmark.sh $(ARGS) 2>$(LOG_FILE_PATH)

run_nginx_benchmark:
	./run_nginx_benchmark.sh 2>$(LOG_FILE_PATH)

run_mysql_benchmark:
	./run_mysql_benchmark.sh 2>$(LOG_FILE_PATH)

generate_trace:
	python3 trans2trace.py

clean:
	rm -rf redis-stable redis-stable.tar.gz $(INTERCEPTOR_SO) $(TRACE_FILE_PATH)