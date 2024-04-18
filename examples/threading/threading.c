#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "threading.h"

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

// Based on https://stackoverflow.com/a/1157217
// millisleep(): Sleep for the requested number of milliseconds.
int millisleep(long milliseconds)
{
    struct timespec ts;
    int res;

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    do {res = nanosleep(&ts, &ts);}
    while (res && errno == EINTR);

    return res;
}

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    
    struct thread_data* threadFuncArgs = (struct thread_data*) thread_param;
    
    millisleep(threadFuncArgs->wait_to_obtain_ms);
    pthread_mutex_lock(threadFuncArgs->mutex);
    millisleep(threadFuncArgs->wait_to_release_ms);
    pthread_mutex_unlock(threadFuncArgs->mutex);
    
    threadFuncArgs->thread_complete_success = true;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * 
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data* threadData = malloc(sizeof(struct thread_data));
    threadData->thread_complete_success = false;
    threadData->mutex = mutex;
    threadData->wait_to_obtain_ms = wait_to_obtain_ms;
    threadData->wait_to_release_ms = wait_to_release_ms;
    
    return 0 == pthread_create(thread, NULL, threadfunc, threadData);
}

