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

#define FOOTHREAD_THREADS_MAX 10
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
    int key;
    int tid;
} foothread_mutex_t;

typedef struct {
    int keys[2];
    int value;
    int threads;
} foothread_barrier_t;