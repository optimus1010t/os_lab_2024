#include "foothread.h"
#define wait(s) semop(s, &pop, 1) 
#define signal(s) semop(s, &vop, 1)

static int joinnable_threads = 0;
static int initialiser = 0;
static int mutexes_and_barriers = 1;

static int sem_join, sem_join_count;
static int sem_mutex;


void foothread_attr_setjointype(foothread_attr_t *attr, int join_type) {
    if(attr==NULL){
        printf("foothread_attr_setjointype: attr is NULL\n");
        return;
    }
    if(join_type==FOOTHREAD_JOINABLE || join_type==FOOTHREAD_DETACHED)
        attr->join_type=join_type;
    else
        printf("foothread_attr_setjointype: join_type is not valid\n");
}
void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size) {
    if(attr==NULL){
        printf("foothread_attr_setstacksize: attr is NULL\n");
        return;
    }
    attr->stack_size = stack_size;
}

int foothread_start_routine(void *arg) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    foothread_t *thread = (foothread_t *)arg;
    thread->start_routine(thread->arg);
	if (thread->leader_pid == getpid()) {
		if (thread->is_joinnable) {
            wait(sem_join_count);
			joinnable_threads--;
            signal(sem_join);
            signal(sem_join_count);
		}
	}
    free(thread);
    return 0;
}

void foothread_exit() {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
	if (getpid() == gettid()){
        wait(sem_join_count);
		while (joinnable_threads > 0) {
            signal(sem_join_count);
            wait(sem_join);
            wait(sem_join_count);
        }
        signal(sem_join_count);
        return;
	}
	else return;
}

void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    if (initialiser == 0 && gettid() == getpid() ){ // even this would require a mutex right ????
        sem_mutex = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
        semctl(sem_mutex, 0, SETVAL, 1);
        sem_join = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
        semctl(sem_join, 0, SETVAL, -(FOOTHREAD_THREADS_MAX+1));
        sem_join_count = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
        semctl(sem_join_count, 0, SETVAL, 1);
        initialiser++;
    }
    foothread_t *my_thread = (foothread_t *)malloc(sizeof(foothread_t));
    if (my_thread == NULL) {
        perror("malloc");
        exit(0);
    }
    if (attr == NULL) {
        attr = &(foothread_attr_t)FOOTHREAD_ATTR_INITIALIZER;
    }

	if (attr->join_type == FOOTHREAD_JOINABLE) my_thread->is_joinnable = 1;
	else my_thread->is_joinnable = 0;

    my_thread->start_routine = start_routine;
    my_thread->arg = arg;
    my_thread->leader_pid = getpid();

	int clone_flags;
	if (attr->join_type == FOOTHREAD_JOINABLE) clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | 0);
	else clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | 0);
    my_thread->tid = clone(foothread_start_routine, malloc(attr->stack_size) + attr->stack_size, clone_flags, my_thread);
	
    if (my_thread->tid == -1) {
        perror("clone");
        free(my_thread);
        exit(0);
    }
    if (thread != NULL) {
		thread->tid = my_thread->tid;
		thread->leader_pid = my_thread->leader_pid;
    }
	if (attr->join_type == FOOTHREAD_JOINABLE) {
        wait(sem_join_count);
		joinnable_threads++;
        signal(sem_join_count);
	}
}


void foothread_mutex_init(foothread_mutex_t *mutex) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    wait(sem_mutex);
    mutex->key[0] = mutexes_and_barriers;
    mutex->key[1] = mutexes_and_barriers + 1;
    mutexes_and_barriers += 2;
    signal(sem_mutex);
    int my_mutex = semget(ftok("tree.txt",mutex->key[0]), 1, 0777|IPC_CREAT);
    semctl(my_mutex, 0, SETVAL, 1);
    my_mutex = semget(ftok("tree.txt",mutex->key[1]), 1, 0777|IPC_CREAT);
    semctl(my_mutex, 0, SETVAL, 1);
    wait(my_mutex);
    for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++) mutex->threads_waiting[i] = -1;
    signal(my_mutex);
    mutex->tid = gettid();
}

void foothread_mutex_lock(foothread_mutex_t *mutex) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    int my_mutex = semget(ftok("tree.txt",mutex->key[1]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    wait(my_mutex);
    for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++) {
        if (mutex->threads_waiting[i] == -1) {
            mutex->threads_waiting[i] = gettid();
            break;
        }
    }
    signal(my_mutex);
    my_mutex = semget(ftok("tree.txt",mutex->key[0]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    wait(my_mutex);
}

void foothread_mutex_unlock(foothread_mutex_t *mutex) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    int my_mutex = semget(ftok("tree.txt",mutex->key[0]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    if (semctl(my_mutex, 0, GETVAL, 0) == 1) {
        printf("This mutex is already unlocked\n");
        exit(0);
    }
    int my_mutex2 = semget(ftok("tree.txt",mutex->key[1]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    wait(my_mutex2);
    for (int i = 0; i < FOOTHREAD_THREADS_MAX; i++) {
        if (mutex->threads_waiting[i] == gettid()) {
            mutex->threads_waiting[i] = -1;
            signal(my_mutex);
            break;
        }
    }
    signal(my_mutex2);
}

void foothread_mutex_destroy(foothread_mutex_t *mutex) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    if (mutex->tid != gettid()) {
        printf("This thread does not own the mutex\n");
        exit(0);
    }
    int my_mutex = semget(ftok("tree.txt",mutex->key[0]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    semctl(my_mutex, 0, IPC_RMID, 0);
    my_mutex = semget(ftok("tree.txt",mutex->key[1]), 1, 0777);
    if (my_mutex == -1) {
        printf("This mutex does not exist\n");
        exit(0);
    }
    semctl(my_mutex, 0, IPC_RMID, 0);
}

void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    if (count == 0) {
        printf("Count cannot be zero\n");
        exit(0);
    }
    wait(sem_mutex);
    barrier->keys[0] = mutexes_and_barriers;
    barrier->keys[1] = mutexes_and_barriers + 1;
    mutexes_and_barriers += 2;
    signal(sem_mutex);
    int my_barrier = semget(ftok("tree.txt",barrier->keys[0]), count, 0777|IPC_CREAT);
    semctl(my_barrier, 0, SETVAL, 0);
    my_barrier = semget(ftok("tree.txt",barrier->keys[1]), 1, 0777|IPC_CREAT);
    semctl(my_barrier, 0, SETVAL, 1);
    wait(my_barrier);
    barrier->value = count;
    barrier->thread_count = 0;
    signal(my_barrier);
}

void foothread_barrier_wait(foothread_barrier_t *barrier) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    int my_barrier = semget(ftok("tree.txt",barrier->keys[0]), 1, 0777);
    if (my_barrier == -1) {
        printf("This barrier does not exist\n");
        exit(0);
    }
    my_barrier = semget(ftok("tree.txt",barrier->keys[1]), 1, 0777);
    if (my_barrier == -1) {
        printf("This barrier does not exist\n");
        exit(0);
    }
    wait(my_barrier);
    barrier->thread_count++;
    if (barrier->thread_count == barrier->value) {
        my_barrier = semget(ftok("tree.txt",barrier->keys[0]), 1, 0777);
        for (int i = 1; i < barrier->value; i++) { signal(my_barrier); }
        barrier->thread_count = 0;
        my_barrier = semget(ftok("tree.txt",barrier->keys[1]), 1, 0777);
        signal(my_barrier);
        return;
    }
    signal(my_barrier);
    my_barrier = semget(ftok("tree.txt",barrier->keys[0]), 1, 0777);
    wait(my_barrier);
}

void foothread_barrier_destroy(foothread_barrier_t *barrier) {
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    int my_barrier = semget(ftok("tree.txt",barrier->keys[0]), 1, 0777);
    if (my_barrier == -1) {
        printf("This barrier does not exist\n");
        exit(0);
    }
    semctl(my_barrier, 0, IPC_RMID, 0);
    my_barrier = semget(ftok("tree.txt",barrier->keys[1]), 1, 0777);
    if (my_barrier == -1) {
        printf("This barrier does not exist\n");
        exit(0);
    }
    semctl(my_barrier, 0, IPC_RMID, 0);
}

