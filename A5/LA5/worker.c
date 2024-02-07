#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define wait_sem(s) semop(s, &pop, 1)
#define signal_sem(s) semop(s, &vop, 1)

int main(int argc, char *argv[])
{
    int n, worker_i, j, i;
    n = atoi(argv[1]);
    worker_i = atoi(argv[2]);

    int shm_adj;
    shm_adj = shmget(ftok("graph.txt", 6), n*n*sizeof(int), 0777);
    int *adjacency = (int *)shmat(shm_adj, 0, 0);

    int key_T, key_idx;
    key_T = ftok("graph.txt", 1);
    key_idx = ftok("graph.txt", 2);
    int shm_T, shm_idx;
    shm_T = shmget(key_T, n*sizeof(int), 0777);
    shm_idx = shmget(key_idx, sizeof(int), 0777);
    int *T = (int *)shmat(shm_T, 0, 0);
    int *idx = (int *)shmat(shm_idx, 0, 0);

    int sync;
    int key;

    key = ftok("graph.txt", 0);
    sync = semget(key, n, 0777);
    int mtx;
    mtx = semget(ftok("graph.txt", 3), 1, 0777);
    int ntfy;
    ntfy = semget(ftok("graph.txt", 4), 1, 0777);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1;

    // wait for all the parents to be in T
    for (i=0; i < n; i++) {
        if (adjacency[i * n + worker_i] == 1) {
            pop.sem_num = vop.sem_num = worker_i;
            wait_sem(sync);
        }
    }
    pop.sem_num = vop.sem_num = 0;
    wait_sem(mtx);
    T[*idx] = worker_i;
    *idx = *idx + 1;
    signal_sem(mtx);
    signal_sem(ntfy);
    // signal all the edges going out of i
    for (j=0; j < n; j++) {
        if (adjacency[worker_i * n + j] == 1) {
            pop.sem_num = vop.sem_num = j;
            signal_sem(sync);
        }
    }
    // detach the shared memory
    shmdt(T);
    shmdt(idx);
    shmdt(adjacency);
    return 0;
}
