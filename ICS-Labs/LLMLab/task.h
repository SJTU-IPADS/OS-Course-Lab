#pragma once

#include "thread-sync.h"
#include <pthread.h>
#include <semaphore.h>

#define NUM_WORKERS 4
#define TASK_QUEUE_SIZE 128
#define BLOCK_SIZE 64 // the number of output channels each task handles
#define NUM_THREADS 4

// task structure
typedef struct {
    float *out;      // output tensor
    float *inp;      // input tensor
    float *weight;   // weight tensor
    float *bias;     // bias tensor
    int b, t;        // batch and token index
    int start_o;     // start output channel
    int end_o;       // end output channel
    int B, T, C, OC; // batch, token length , channel, output channel
    int valid;       // valid flag
} MatMulTask;        // matrix multiplication task

// task queue structure
typedef struct {
    MatMulTask tasks[TASK_QUEUE_SIZE]; // task queue
    int front, rear;                   // front and rear of task queue
    int size;                          // size of task queue
    mutex_t mutex;                     // mutex lock for task queue
    cond_t not_empty;                  // condition variable: queue is not empty
    cond_t not_full;                   // condition variable: queue is not full
    int stop;                          // stop flag for task queue
    int completed_tasks;               // the number of completed tasks
    int total_tasks;                   // the total number of tasks
    sem_t done;                        // used to wait for all tasks to complete
} TaskQueue;

// initialize the task queue
void init_task_queue();

// enqueue a task to the task queue
void enqueue_task(MatMulTask task);

// dequeue a task from the task queue
MatMulTask dequeue_task();

// process a matrix multiplication task
void process_matmul_task(MatMulTask *task);

// worker thread function
void *worker_thread(void *arg);

// stop the task queue and all worker threads
void stop_task_queue();
