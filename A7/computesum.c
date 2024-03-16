#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int subthread(void *arg) {
    int *args = (int *) arg;
    int index = args[0]; int parent = args[1]; int children = args[2]; int n = args[3];
    int shm_sum = shmget(ftok("tree.txt", 's'), n*sizeof(int), 0);
    int *sum = (int *) shmat(shm_sum, 0, 0);

    int shm_barrier = shmget(ftok("tree.txt", 'b'), n*sizeof(foothread_barrier_t), 0);
    foothread_barrier_t *barrier = (foothread_barrier_t *) shmat(shm_barrier, 0, 0);

    int shm_mutex = shmget(ftok("tree.txt", 'm'), n*sizeof(foothread_mutex_t), 0);
    foothread_mutex_t *mutex = (foothread_mutex_t *) shmat(shm_mutex, 0, 0);
    foothread_barrier_wait(&barrier[index]);
    if (children == 0) {
        printf("Leaf node \t%d :: Enter a positive integer: ", index);
        int x; scanf("%d", &x);
        foothread_mutex_lock(&mutex[parent]);
        sum[parent] += x;
        foothread_mutex_unlock(&mutex[parent]);
        foothread_barrier_wait(&barrier[parent]);
        return (0);
    }
    if (index == parent) {
        foothread_barrier_wait(&barrier[index]);
        printf("Sum at root (node %d) = %d", index, sum[index]);
        return (0);
    }    
    printf("Internal node \t%d gets the partial sum %d from its children", index, sum[index]);
    foothread_mutex_lock(&mutex[parent]);
    sum[parent] += sum[index];
    foothread_mutex_unlock(&mutex[parent]);
    foothread_barrier_wait(&barrier[parent]);
    return (0);
}

int main () {
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
    for (int i = 0; i < n; i++) if (tree[i] > 0 && tree[i] < n && tree[i]!=i) child[tree[i]]++;
    foothread_t threads[n];
    foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;

    int arg[n][4];
    for (int i = 0; i < n; i++) { arg[i][0] = i; arg[i][1] = tree[i]; arg[i][2] = child[i]; arg[i][3] = n;}

    int shm_sum = shmget(ftok("tree.txt", 's'), n*sizeof(int), 0777|IPC_CREAT);
    int *sum = (int *) shmat(shm_sum, 0, 0);

    int shm_barrier = shmget(ftok("tree.txt", 'b'), n*sizeof(foothread_barrier_t), 0777|IPC_CREAT);
    foothread_barrier_t *barrier = (foothread_barrier_t *) shmat(shm_barrier, 0, 0);

    int shm_mutex = shmget(ftok("tree.txt", 'm'), n*sizeof(foothread_mutex_t), 0777|IPC_CREAT);
    foothread_mutex_t *mutex = (foothread_mutex_t *) shmat(shm_mutex, 0, 0);

    for (int i = 0; i < n; i++) {
        foothread_barrier_init(&barrier[i], (child[i]+1));
        foothread_mutex_init(&mutex[i]);
    }

    for (int i = 0; i < n; i++) {
        sum[i] = 0;
    }
    for (int i = 0; i < n; i++) foothread_create(&threads[i], &attr, subthread, (void *) &arg[i]);
    foothread_exit();
    // cleanup
    return 0;
}