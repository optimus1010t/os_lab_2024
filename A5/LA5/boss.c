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

int main()
{
    FILE *file;
    int n, i, j;
    // Open the file
    file = fopen("graph.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    // Read the number of nodes
    fscanf(file, "%d", &n);
    // create adjacency matrix to be shared
    int shm_adj;
    shm_adj = shmget(ftok("graph.txt", 6), n*n*sizeof(int), 0777 | IPC_CREAT);
    int *adjacency = (int *)shmat(shm_adj, 0, 0);
    // Read the adjacency matrix
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            fscanf(file, "%d", &adjacency[i * n + j]);
        }
    }
    // Close the file
    fclose(file);

    int key_T, key_idx;
    key_T = ftok("graph.txt", 1);
    key_idx = ftok("graph.txt", 2);
    int shm_T, shm_idx;
    shm_T = shmget(key_T, n*sizeof(int), 0777 | IPC_CREAT);
    shm_idx = shmget(key_idx, sizeof(int), 0777 | IPC_CREAT);
    int *T = (int *)shmat(shm_T, 0, 0);
    int *idx = (int *)shmat(shm_idx, 0, 0);

    // Create the semaphores
    int sync;
    int key;
    key = ftok("graph.txt", 0);
    sync = semget(key, n, 0777 | IPC_CREAT);
    for (i = 0; i < n; i++) {
        semctl(sync, i, SETVAL, 0);
    }
    int mtx;
    mtx = semget(ftok("graph.txt", 3), 1, 0777 | IPC_CREAT);
    semctl(mtx, 0, SETVAL, 1);
    int ntfy;
    ntfy = semget(ftok("graph.txt", 4), 1, 0777 | IPC_CREAT);
    semctl(ntfy, 0, SETVAL, 0);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1;

    // Initialize the shared memory
    for (i = 0; i < n; i++) {
        T[i] = -1;
    }
    *idx = 0;
    printf("+++ Boss: Setup done...\n");
    printf("+++ Topological sorting of the vertices\n");
    // wait for ntfy for all n nodes
    for (i=0; i < n; i++) {
        wait_sem(ntfy);
    }
    // print the topological sorting
    for (i = 0; i < n; i++) {
        printf("%d ", T[i]);
    }
    // check if it is a valid topological sort
    int flag = 0;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            if (adjacency[T[j] * n + T[i]] == 1) {
                // There's an incoming edge from T[j] to T[i]
                flag = 1;
                break;
            }
        }
    }
    if (flag == 0) printf("\n+++ Boss: Well done, my team...\n");
    else printf("\n+++ Boss: You have failed me, my team...\n");
    // detach shared memory
    shmdt(T);
    shmdt(idx);
    // remove shared memory
    shmctl(shm_T, IPC_RMID, 0);
    shmctl(shm_idx, IPC_RMID, 0);
    // remove semaphores
    semctl(sync, 0 , IPC_RMID, 0);
    semctl(mtx, 0, IPC_RMID, 0);
    semctl(ntfy, 0, IPC_RMID, 0);
    // detach shared memory
    shmdt(adjacency);
    // remove shared memory
    shmctl(shm_adj, IPC_RMID, 0);
    return 0;
}