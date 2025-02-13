#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <process.h>
#include <stdint.h>
#include "pthread.h"

struct thread_ctx {
  void* (*entry)(void* arg);
  void* arg;
  pthread_t self;
};

int pthread_once(pthread_once_t* guard, void (*callback)(void)) {
  if (guard->ran) {
    return 0;
  }

  DWORD result;
  HANDLE existing_event, created_event;

  created_event = CreateEvent(NULL, 1, 0, NULL);
  if (created_event == 0) {
    errno = ENOMEM;
    return ENOMEM;
  }

  existing_event = InterlockedCompareExchangePointer(&guard->event,
                                                     created_event,
                                                     NULL);

  if (existing_event == NULL) {
    callback();

    result = SetEvent(created_event);
    assert(result);
    guard->ran = 1;

  } else {
    CloseHandle(created_event);
    result = WaitForSingleObject(existing_event, INFINITE);
    assert(result == WAIT_OBJECT_0);
  }
  return 0;
}

int pthread_mutex_lock (pthread_mutex_t *mutex) {
  EnterCriticalSection(mutex);
  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  LeaveCriticalSection(mutex);
  return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
  InitializeCriticalSection(mutex);
  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  DeleteCriticalSection(mutex);
  return 0;
}

int pthread_attr_init(pthread_attr_t *attr) {
  if (!attr) return 1;
  memset(attr, 0, sizeof(pthread_attr_t));
  return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
  if (!attr) return 1;
  attr->detachstate = detachstate;
  return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
  (void) attr;
  return 0;
}

static unsigned __stdcall thread_start(void* arg) {
  struct thread_ctx *ctx_p;
  struct thread_ctx ctx;

  ctx_p = arg;
  ctx = *ctx_p;
  free(ctx_p);

  ctx.entry(ctx.arg);

  return 0;
}

int pthread_create(pthread_t* newthread,
                   const pthread_attr_t *attr,
                   void* (*start_routine)(void*),
                   void* arg) {
  struct thread_ctx* ctx;
  int err;
  HANDLE thread;
  SYSTEM_INFO sysinfo;
  size_t stack_size;
  size_t pagesize;
  int detachstate;

  if (!newthread || !start_routine) return EINVAL;
  stack_size = attr != NULL ? attr->stacksize : 0;
  detachstate = attr != NULL ? attr->detachstate : 0;

  if (stack_size != 0) {
    GetNativeSystemInfo(&sysinfo);
    pagesize = (size_t)sysinfo.dwPageSize;
    /* Round up to the nearest page boundary. */
    stack_size = (stack_size + pagesize - 1) &~ (pagesize - 1);

    if ((unsigned)stack_size != stack_size)
      return EINVAL;
  }

  ctx = malloc(sizeof(*ctx));
  if (ctx == NULL)
    return ENOMEM;

  ctx->entry = start_routine;
  ctx->arg = arg;

  /* Create the thread in suspended state so we have a chance to pass
   * its own creation handle to it */
  thread = (HANDLE) _beginthreadex(NULL,
                                   (unsigned)stack_size,
                                   thread_start,
                                   ctx,
                                   CREATE_SUSPENDED,
                                   NULL);
  if (thread == NULL) {
    err = errno;
    free(ctx);
  } else {
    err = 0;
    *newthread = thread;
    ctx->self = thread;
    ResumeThread(thread);
    if (detachstate == PTHREAD_CREATE_DETACHED) {
      CloseHandle(thread);
    }
  }

  return err;
}

int pthread_join(pthread_t th, void **thread_return) {
  if (WaitForSingleObject(th, INFINITE))
    return GetLastError();
  else {
    CloseHandle(th);
    MemoryBarrier();
    return 0;
  }
}

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr) {
  InitializeConditionVariable(cond);
  return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
  if (!SleepConditionVariableCS(cond, mutex, INFINITE))
    abort();
  return 0;
}

int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* timeout) {
  if (SleepConditionVariableCS(cond, mutex, (DWORD)(timeout->tv_nsec / 1e6)))
    return 0;
  if (GetLastError() != ERROR_TIMEOUT)
    abort();
  return ETIMEDOUT;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
  (void) &cond;
  return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
  WakeConditionVariable(cond);
  return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
  WakeAllConditionVariable(cond);
  return 0;
}

LARGE_INTEGER
getFILETIMEoffset()
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

#define POW10_7                 10000000
#define DELTA_EPOCH_IN_100NS    INT64_C(116444736000000000)

int clock_gettime(int clock_id, struct timespec *tp)
{
    uint64_t t;
    LARGE_INTEGER pf, pc;
    union {
        uint64_t u64;
        FILETIME ft;
    }  ct, et, kt, ut;

    switch(clock_id) {
    case CLOCK_REALTIME:
        {
            GetSystemTimeAsFileTime(&ct.ft);
            t = ct.u64 - DELTA_EPOCH_IN_100NS;
            tp->tv_sec = t / POW10_7;
            tp->tv_nsec = ((int) (t % POW10_7)) * 100;

            return 0;
        }

    default:
        break;
    }

    errno = EINVAL;
    return EINVAL;
}
