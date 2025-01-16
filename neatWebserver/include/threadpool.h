#ifndef THREADPOOL
#define THREADPOOL
#include "requestData.h"
#include <pthread.h>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

typedef enum 
{
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
struct threadpool_t
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;//  所有started线程的id,一个数组
    threadpool_task_t *queue;// 因为一个线程需要处理很多个连接的任务，所有线程的任务都存放在这个队列中
    int thread_count;//和started作用差不多，一般都是设定的线程数量
    int queue_size; // 任务队列的长度是固定的。因为一个线程需要处理很多个连接的任务，所以有任务队列
    int head; //queue循环数组头部
    int tail; //queue的尾部
    int count; //有多少个任务等待处理，后续应该被分配到每个线程中
    int shutdown;
    int started;//和started作用差不多，一般都是设定的线程数量
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);
static void *threadpool_thread(void *threadpool);

#endif