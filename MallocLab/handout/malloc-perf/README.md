# Malloc Performance Testing Framework

This directory contains a reference implementation for measuring malloc performance using a function interception approach. It provides a framework to evaluate malloc implementations with real-world applications like Redis, MySQL, and Nginx.

> **Note**: It is recommended to run these tests in a Docker container with the `ics` image.

## Overview

This framework uses function interception to monitor malloc calls in real applications. While this is one approach to measure malloc performance, you are encouraged to explore and implement other methodologies.

## Directory Structure

```
malloc-perf/
├── Makefile                 # Main build system configuration
├── malloc_interceptor.c     # Source code for malloc function interception
├── malloc_interceptor.so    # Compiled shared library for interception
├── compile_interceptor.sh   # Script to build the interceptor
├── trans2trace.py          # Python script for trace processing
├── setup_redis.sh          # Redis installation and setup
├── setup_mysql.sh          # MySQL installation and setup
├── setup_nginx.sh          # Nginx installation and setup
├── run_redis_benchmark.sh  # Redis benchmark script
├── run_mysql_benchmark.sh  # MySQL benchmark script
├── run_nginx_benchmark.sh  # Nginx benchmark script
├── nginx.conf          # Nginx configuration file
└── index.html          # Test webpage for Nginx

```

## Components

- `malloc_interceptor.c`: Source code for the malloc interceptor
- `compile_interceptor.sh`: Script to compile the malloc interceptor
- Setup scripts for different applications:
  - `setup_redis.sh`
  - `setup_mysql.sh`
  - `setup_nginx.sh`
- Benchmark scripts:
  - `run_redis_benchmark.sh`
  - `run_mysql_benchmark.sh`
  - `run_nginx_benchmark.sh`
- `trans2trace.py`: Script to transform raw data into traces
- Supporting files:
  - `nginx.conf`: Nginx configuration
  - `index.html`: Sample web page for Nginx testing

## Build System (Makefile)

The project includes a Makefile that automates the entire testing process. Here are the main targets:

```bash
make all              # Run complete test suite (setup, compile, benchmark, but not generate trace)
make setup_redis      # Set up Redis environment
make setup_nginx      # Set up Nginx environment
make setup_mysql      # Set up MySQL environment
make compile_interceptor  # Build the malloc interceptor
make run_redis_benchmark # Run Redis benchmarks
make run_nginx_benchmark # Run Nginx benchmarks
make run_mysql_benchmark # Run MySQL benchmarks
make generate_trace   # Process results into trace file
make clean           # Clean up generated files and installations
```

## Usage

You can either use individual commands or the Makefile targets:

1. Complete test suite:
   ```bash
   make all
   ```

2. Individual steps:
   ```bash
   make setup_redis
   make setup_mysql
   make setup_nginx
   make compile_interceptor
   make run_redis_benchmark
   make run_mysql_benchmark
   make run_nginx_benchmark
   make generate_trace
   ```

3. Manual execution (alternative to make):
   ```bash
   ./compile_interceptor.sh
   ./setup_redis.sh   # For Redis
   ./setup_mysql.sh   # For MySQL
   ./setup_nginx.sh   # For Nginx
   ./run_redis_benchmark.sh   # For Redis
   ./run_mysql_benchmark.sh   # For MySQL
   ./run_nginx_benchmark.sh   # For Nginx
   python3 trans2trace.py
   ```

## Important Notes

1. This is a reference implementation. You are encouraged to develop your own methods for measuring malloc performance.
2. The function interception approach used here is just one way to measure malloc behavior.
3. Consider factors like:
   - Memory usage patterns
   - Allocation/deallocation frequency
   - Memory fragmentation
   - Performance impact
4. All benchmark results will be logged to the path specified in `LOG_FILE_PATH`
5. Processed traces will be saved to the path specified in `TRACE_FILE_PATH`

## Docker Environment

It is recommended to run these tests in a Docker container using the `ics` image to ensure a consistent testing environment and avoid potential system-level conflicts. The provided paths and configurations are optimized for the `ics` container environment. 