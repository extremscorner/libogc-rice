
#ifndef PTHREAD_H
#define PTHREAD_H

#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/cond.h>

typedef lwp_t pthread_t;
typedef void* pthread_attr_t;

typedef mutex_t pthread_mutex_t;
typedef void* pthread_mutexattr_t;

typedef cond_t pthread_cond_t;
typedef void* pthread_condattr_t;

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg) {
	return LWP_CreateThread(thread, start_routine, arg, NULL, 0, LWP_PRIO_NORMAL);
}
static inline int pthread_join(pthread_t thread, void **value_ptr) {
	return LWP_JoinThread(thread, value_ptr);
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	return LWP_MutexInit(mutex, false);
}
static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
	return LWP_MutexDestroy(*mutex);
}
static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
	return LWP_MutexLock(*mutex);
}
static inline int pthread_mutex_trylock(pthread_mutex_t *mutex) {
	return LWP_MutexTryLock(*mutex);
}
static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
	return LWP_MutexUnlock(*mutex);
}

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
	return LWP_CondInit(cond);
}
static inline int pthread_cond_destroy(pthread_cond_t *cond) {
	return LWP_CondDestroy(*cond);
}
static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	return LWP_CondWait(*cond, *mutex);
}
static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
	return LWP_CondTimedWait(*cond, *mutex, abstime);
}
static inline int pthread_cond_signal(pthread_cond_t *cond) {
	return LWP_CondSignal(*cond);
}
static inline int pthread_cond_broadcast(pthread_cond_t *cond) {
	return LWP_CondBroadcast(*cond);
}

#endif /* PTHREAD_H */
