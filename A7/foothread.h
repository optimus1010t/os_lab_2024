#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <sys/syscall.h>

#define FOOTHREAD_THREADS_MAX 1000
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152
#define FOOTHREAD_JOINABLE 1
#define FOOTHREAD_DETACHED 0
#define FOOTHREAD_ATTR_INITIALIZER {FOOTHREAD_JOINABLE, FOOTHREAD_DEFAULT_STACK_SIZE}
#define MAXMUTEXES 20

typedef struct {
    int join_type;
    size_t stack_size;
} foothread_attr_t;

typedef struct {
    pid_t leader_pid;
    pid_t tid;
    int (*start_routine)(void *);
    void *arg;
	int is_joinnable;
} foothread_t;

typedef struct {
    int key[2];
    int tid;
    int threads_waiting[FOOTHREAD_THREADS_MAX];
} foothread_mutex_t;

typedef struct {
    int keys[2];
    int value;
    int thread_count;
} foothread_barrier_t;

void foothread_attr_setjointype(foothread_attr_t *attr, int join_type);
void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size);
void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg);
void foothread_exit();
void foothread_mutex_init(foothread_mutex_t *mutex);
void foothread_mutex_lock(foothread_mutex_t *mutex);
void foothread_mutex_unlock(foothread_mutex_t *mutex);
void foothread_mutex_destroy(foothread_mutex_t *mutex);
void foothread_barrier_init(foothread_barrier_t *barrier, int value);
void foothread_barrier_wait(foothread_barrier_t *barrier);
void foothread_barrier_destroy(foothread_barrier_t *barrier);