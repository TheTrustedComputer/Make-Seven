/*
    Copyright (C) 2020- TheTrustedComputer
    
    A compatibility layer that maps the C11 thread functions to POSIX threads for use with MinGW.
    We will only implement a subset of the functions we need; support for others is not provided.
 */

#ifndef MINGW_THREADS_H
#define MINGW_THREADS_H

#include <sched.h>
#include <errno.h>
#include <pthread.h>

// Aliases
typedef pthread_t thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t cnd_t;
typedef int(thrd_start_t)(void*);

// Mutex types
enum
{
    mtx_plain = 1,
    mtx_timed = 2,
    mtx_recursive = 4
};

// Thread error status
enum
{
    thrd_success,
    thrd_error,
    thrd_nomem,
};

// Structure to pass arguments expected by C11 threads
typedef struct
{
    thrd_start_t *func;
    void *arg;
}
thrd_start_wrapper_arg_t;

// Thread functions
int thrd_create(thrd_t*, thrd_start_t*, void*);
int thrd_join(thrd_t, int*);
int thrd_sleep(const struct timespec*, struct timespec*);
void thrd_yield(void);

// Wrapper function; C11 threads return an int instead of a void pointer
void *thrd_start_wrapper(void*);

// Mutual exclusion
int mtx_init(mtx_t*, int);
void mtx_destroy(mtx_t*);
int mtx_lock(mtx_t*);
int mtx_unlock(mtx_t*);

// Condition variables
int cnd_init(cnd_t*);
void cnd_destroy(cnd_t*);
int cnd_signal(cnd_t*);
int cnd_broadcast(cnd_t*);
int cnd_wait(cnd_t*, mtx_t*);

#endif /* MINGW_THREADS_H */
