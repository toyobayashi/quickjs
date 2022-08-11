#ifndef WIN_PTHREAD_H_
#define WIN_PTHREAD_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stddef.h>
#include <time.h>
#include <winsock.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  PTHREAD_CREATE_JOINABLE,
#define PTHREAD_CREATE_JOINABLE	PTHREAD_CREATE_JOINABLE
  PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED	PTHREAD_CREATE_DETACHED
};

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

typedef struct {
  int detachstate;
  size_t stacksize;
} pthread_attr_t;

typedef union {
  char __size[4];
  int __align;
} pthread_mutexattr_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;

int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_destroy(pthread_attr_t *attr);

int pthread_create(pthread_t* newthread,
                   const pthread_attr_t *attr,
                   void* (*start_routine)(void*),
                   void* arg);
int pthread_join(pthread_t th, void **thread_return);

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* timeout);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

#define CLOCK_REALTIME 0
int clock_gettime(int X, struct timeval *tv);

#ifdef __cplusplus
}
#endif

#endif
