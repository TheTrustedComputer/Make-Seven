/*
    Copyright (C) 2020- TheTrustedComputer
*/

#if (defined(__MINGW64__) || defined(__MINGW32__))

#include <stdio.h>
#include <stdlib.h>

#include "mingw_threads.h"

int thrd_create(thrd_t *_thr, thrd_start_t *_func, void *_arg)
{
    thrd_start_wrapper_arg_t *wrapper;
    
    if (!(wrapper = malloc(sizeof(*wrapper))))
    {
        return thrd_nomem;
    }
    
    wrapper->func = _func;
    wrapper->arg = _arg;
    
    switch (pthread_create(_thr, NULL, thrd_start_wrapper, wrapper))
    {
    case 0:
        return thrd_success;
    case EAGAIN:
        return thrd_nomem;
    default:
        return thrd_error;
    }
}

int thrd_join(thrd_t _thr, int *_res)
{
    void *res;
    int joinErr;
    
    joinErr = pthread_join(_thr, &res);
    
    if (_res && res)
    {
       *_res = *(int*)(res);
    }
    
    return joinErr ? thrd_error : thrd_success;
}

int thrd_sleep(const struct timespec *_DUR, struct timespec *_rem)
{
    return nanosleep(_DUR, _rem);
}

void thrd_yield(void)
{
    sched_yield();
}

void *thrd_start_wrapper(void *_arg)
{
    thrd_start_wrapper_arg_t *wrapper;
    int return_stat;
    
    wrapper = _arg;
    return_stat = wrapper->func(wrapper->arg);
    free(wrapper);
    
    return (void*)(return_stat);
}

int mtx_init(mtx_t *_mutex, int _type)
{
    int mtxErr;
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    
    switch (_type)
    {
    case mtx_plain:
    case mtx_timed:
        if (_type & mtx_recursive)
        {
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        }
        else
        {
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
        }
        break;
    default:
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
    }
    
    mtxErr = pthread_mutex_init(_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    return mtxErr ? thrd_error : thrd_success;
}

void mtx_destroy(mtx_t *_mutex)
{
    pthread_mutex_destroy(_mutex);
}

int mtx_lock(mtx_t *_mutex)
{
    return pthread_mutex_lock(_mutex) ? thrd_error : thrd_success;
}

int mtx_unlock(mtx_t *_mutex)
{
    return pthread_mutex_unlock(_mutex) ? thrd_error : thrd_success;
}

int cnd_init(cnd_t *_cond)
{
    switch (pthread_cond_init(_cond, NULL))
    {
    case 0:
        return thrd_success;
    case ENOMEM:
    case EAGAIN:
        return thrd_nomem;
    default:
        return thrd_error;
    }
}

void cnd_destroy(cnd_t *_cond)
{
    pthread_cond_destroy(_cond);
}

int cnd_signal(cnd_t *_cond)
{
    return pthread_cond_signal(_cond) ? thrd_error : thrd_success;
}

int cnd_broadcast(cnd_t *_cond)
{
    return pthread_cond_broadcast(_cond) ? thrd_error : thrd_success;
}

int cnd_wait(cnd_t *_cond, mtx_t *_mutex)
{
    return pthread_cond_wait(_cond, _mutex) ? thrd_error : thrd_success;
}

#endif
