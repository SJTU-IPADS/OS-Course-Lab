#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * config.h - malloc lab configuration file
 *
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */

/*
 * This is the default path where the driver will look for the
 * default tracefiles. You can override it at runtime with the -t flag.
 */
#define TRACEDIR "./traces/"

/*
 * This is the list of default tracefiles in TRACEDIR that the driver
 * will use for testing. Modify this if you want to add or delete
 * traces from the driver's test suite. For example, if you don't want
 * your students to implement realloc, you can delete the last two
 * traces.
 */
#define DEFAULT_TRACEFILES \
  "amptjp-bal.rep",\
  "cccp-bal.rep",\
  "cp-decl-bal.rep",\
  "expr-bal.rep",\
  "coalescing-bal.rep",\
  "random-bal.rep",\
  "random2-bal.rep",\
  "binary-bal.rep",\
  "binary2-bal.rep",\
  "realloc-bal.rep",\
  "realloc2-bal.rep",\
  "mysql.rep",\
  "nginx.rep",\
  "redis.rep"

/**********************
 * The weight of each testcase
 **********************/

// Add the array for trace files and their weights
static struct {
    char *filename;
    int weight;
} trace_weights[] = {
    {"amptjp-bal.rep", 0},
    {"amptjp.rep", 0},
    {"binary2-bal.rep", 0},
    {"binary2.rep", 0},
    {"binary-bal.rep", 0},
    {"binary.rep", 0},
    {"cccp-bal.rep", 0},
    {"cccp.rep", 0},
    {"coalescing-bal.rep", 0},
    {"coalescing.rep", 0},
    {"cp-decl-bal.rep", 0},
    {"cp-decl.rep", 0},
    {"expr-bal.rep", 0},
    {"expr.rep", 0},
    {"mysql.rep", 2},
    {"nginx.rep", 2},
    {"random2-bal.rep", 0},
    {"random2.rep", 0},
    {"random-bal.rep", 0},
    {"random.rep", 0},
    {"realloc2-bal.rep", 0},
    {"realloc2.rep", 0},
    {"realloc-bal.rep", 0},
    {"realloc.rep", 0},
    {"redis.rep", 2},
    {"short1-bal.rep", 0},
    {"short1.rep", 0},
    {"short2-bal.rep", 0},
    {"short2.rep", 0},
    {NULL, 0}
};

// 为每个测试文件定义util阈值
static struct {
    char *filename;
    double util_threshold;  // 达到或超过这个值就算满分
} trace_utils[] = {
    {"amptjp-bal.rep", 1},  // 示例值，你需要根据实际情况调整
    {"cccp-bal.rep", 1},
    {"cp-decl-bal.rep", 1},
    {"expr-bal.rep", 1},
    {"coalescing-bal.rep", 1},
    {"random-bal.rep", 1},
    {"random2-bal.rep", 1},
    {"binary-bal.rep", 1},
    {"binary2-bal.rep", 1},
    {"realloc-bal.rep", 1},
    {"realloc2-bal.rep", 1},
    {"mysql.rep", 0.95},
    {"nginx.rep", 0.95},
    {"redis.rep", 0.85},
    {NULL, 0.0}
};

/*
 * This constant gives the estimated performance of the libc malloc
 * package using our traces on some reference system, typically the
 * same kind of system the students use. Its purpose is to cap the
 * contribution of throughput to the performance index. Once the
 * students surpass the AVG_LIBC_THRUPUT, they get no further benefit
 * to their score.  This deters students from building extremely fast,
 * but extremely stupid malloc packages.
 */
#define AVG_LIBC_THRUPUT      5E6  /* 5E6 ops/sec */

 /* 
  * This constant determines the contributions of space utilization
  * (UTIL_WEIGHT) and throughput (1 - UTIL_WEIGHT) to the performance
  * index.  
  */
#define UTIL_WEIGHT .60

/* 
 * Alignment requirement in bytes (either 4 or 8) 
 */
#define ALIGNMENT 8  

/* 
 * Maximum heap size in bytes 
 */
#define MAX_HEAP (20*(1<<20))  /* 20 MB */

/*****************************************************************************
 * Set exactly one of these USE_xxx constants to "1" to select a timing method
 *****************************************************************************/
#define USE_FCYC   0   /* cycle counter w/K-best scheme (x86 & Alpha only) */
#define USE_ITIMER 0   /* interval timer (any Unix box) */
#define USE_GETTOD 1   /* gettimeofday (any Unix box) */

#endif /* __CONFIG_H */
