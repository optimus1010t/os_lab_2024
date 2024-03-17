#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct arg {
    int index;
    int parent;
    int children;
    int n;
};

int subthread(void *arg1) {
    // signal(SIGINT, sighandler);
    struct arg *args = (struct arg *) arg1;
    int index = args->index; int parent = args->parent; int children = args->children; int n = args->n;
    int shm_sum = shmget(ftok("tree.txt", 1000+'s'), n*sizeof(int), 0);
    int *sum = (int *) shmat(shm_sum, 0, 0);

    int shm_barrier = shmget(ftok("tree.txt", 1000+'b'), n*sizeof(foothread_barrier_t), 0);
    foothread_barrier_t *barrier = (foothread_barrier_t *) shmat(shm_barrier, 0, 0);

    int shm_mutex = shmget(ftok("tree.txt", 1000+'m'), n*sizeof(foothread_mutex_t), 0);
    foothread_mutex_t *mutex = (foothread_mutex_t *) shmat(shm_mutex, 0, 0);
    foothread_barrier_wait(&barrier[index]);
    if (index == parent) {
        printf("Sum at root (node %d) = %d\n", index, sum[index]);
        foothread_exit(0);
        return 0;
    }    
    if(children!=0) printf("Internal node \t%d gets the partial sum %d from its children\n", index, sum[index]);
    foothread_mutex_lock(&mutex[parent]);
    sum[parent] += sum[index];
    foothread_mutex_unlock(&mutex[parent]);
    foothread_barrier_wait(&barrier[parent]);

    shmdt(sum);
    shmdt(barrier);
    shmdt(mutex);
    foothread_exit();
    return 0;
}

int main () {
    // signal(SIGINT, sighandler);
    int n;
    FILE* fp = fopen("tree.txt", "r");
    fscanf(fp, "%d", &n);
    int tree[n];
    for (int i = 0; i < n; i++) {
        int a, b; fscanf(fp, "%d %d", &a, &b);
        tree[a] = b;
    }
    fclose(fp);
    int child[n];
    for (int i = 0; i < n; i++) child[i] = 0;
    for (int i = 0; i < n; i++) if (tree[i] > -1 && tree[i] < n && tree[i]!=i) child[tree[i]]++;
    foothread_t threads[n];
    foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;

    struct arg *args;
    args = (struct arg *) malloc(n*sizeof(struct arg));
    for (int i = 0; i < n; i++) { args[i].index = i; args[i].parent = tree[i]; args[i].children = child[i]; args[i].n = n;}

    int shm_sum = shmget(ftok("tree.txt", 1000+'s'), n*sizeof(int), 0777|IPC_CREAT);
    int *sum = (int *) shmat(shm_sum, 0, 0);

    int shm_barrier = shmget(ftok("tree.txt", 1000+'b'), n*sizeof(foothread_barrier_t), 0777|IPC_CREAT);
    foothread_barrier_t *barrier = (foothread_barrier_t *) shmat(shm_barrier, 0, 0);

    int shm_mutex = shmget(ftok("tree.txt", 1000+'m'), n*sizeof(foothread_mutex_t), 0777|IPC_CREAT);
    foothread_mutex_t *mutex = (foothread_mutex_t *) shmat(shm_mutex, 0, 0);

    for (int i = 0; i < n; i++) {
        foothread_barrier_init(&barrier[i], (child[i]+1));
        foothread_mutex_init(&mutex[i]);
    }

    for (int i = 0; i < n; i++) {
        sum[i] = 0;
        if(child[i]==0){
            printf("Leaf node \t%d :: Enter a positive integer: ", i);
            if (scanf("%d", &sum[i]) < 0) {
                printf("Invalid input\n");
                return 0;
            }
        }
    }

    
    for (int i = 0; i < n; i++) foothread_create(&threads[i], &attr, subthread, (void *) &args[i]);
    foothread_exit();

    for (int i = 0; i < n; i++) {
            foothread_barrier_destroy(&barrier[i]);
            foothread_mutex_destroy(&mutex[i]);
    }

    //cleanup everything
    free(args);
    shmdt(sum);
    shmdt(barrier);
    shmdt(mutex);   
    shmctl(shm_barrier, IPC_RMID, 0);
    shmctl(shm_mutex, IPC_RMID, 0);
    return 0;
}