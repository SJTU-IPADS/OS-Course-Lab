#include "task.h"

TaskQueue task_queue;                  // task queue
pthread_t worker_threads[NUM_WORKERS]; // worker threads

// initialize the task queue
void init_task_queue()
{
    // TODO BEGIN
    // 1. initialise the elements of the task queue
    // 2. initialise the worker threads

    // TODO END
}

// enqueue a task to the task queue
void enqueue_task(MatMulTask task)
{
    // TODO BEGIN
    // 1. lock the task queue
    // 2. enqueue a task to the task queue
    // 3. notify the consumer
    // 4. unlock the task queue

    // TODO END
}

// dequeue a task from the task queue
MatMulTask dequeue_task()
{
    // TODO BEGIN
    // 1. lock the task queue
    // 2. dequeue a task from the task queue
    // 3. notify the producer
    // 4. unlock the task queue
    // 5. return the task

    // TODO END
}

// process a task
void process_matmul_task(MatMulTask *task)
{

    float *out_bt = task->out + task->b * task->T * task->OC + task->t * task->OC;
    float *inp_bt = task->inp + task->b * task->T * task->C + task->t * task->C;

    // calculate the values of a group of output channels
    for (int o = task->start_o; o < task->end_o; o++) {
        float val = (task->bias != NULL) ? task->bias[o] : 0.0f;
        float *wrow = task->weight + o * task->C;
        for (int i = 0; i < task->C; i++) {
            val += inp_bt[i] * wrow[i];
        }
        out_bt[o] = val;
    }

    // TODO BEGIN
    // 1. lock the task queue
    // 2. update the completed tasks count, if the completed tasks count is equal to the total tasks count, notify the
    // producer
    // 3. unlock the task queue

    // TODO END
}

// worker thread
void *worker_thread(void *arg)
{
    // TODO BEGIN
    // 1. dequeue a task from the task queue
    // 2. process the task
    // 3. unlock the task queue
    // 4. repeat until the task queue is stopped

    // TODO END
}

void stop_task_queue()
{
    // TODO BEGIN
    // 1. lock the task queue
    // 2. stop the task queue
    // 3. notify all the worker threads
    // 4. unlock the task queue
    // 5. wait for all the worker threads to finish

    // TODO END
}