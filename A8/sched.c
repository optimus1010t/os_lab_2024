#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<time.h>

#define wait(s) semop(s, &pop, 1) 
#define signall(s) semop(s, &vop, 1)

struct msgbuf {
    long mtype;
    int msg;
};

int main(int argc, char *argv[]){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    int keymq1, keymq2;
    keymq1=atoi(argv[1]);
    keymq2=atoi(argv[2]);
    int mq1, mq2;
    mq1 = msgget(keymq1, IPC_CREAT|0666);
    mq2 = msgget(keymq2, IPC_CREAT|0666);

    int keysem1 = ftok("Master.c", 'F');            // might pass this as cmd line arg
    int sem1 = semget(keysem1, 1, IPC_CREAT|0666);

    // stores k
    int shm_k_id = shmget(ftok("MMU.c",'A'),sizeof(int),IPC_CREAT|0666);
    int *shm_k_ptr = (int*)shmat(shm_k_id,NULL,0);

    int numOfTerminatedProcesses = 0;       // ????need to make this shared + use semaphores for mutex

    struct msgbuf buf;

    while(numOfTerminatedProcesses < *shm_k_ptr){
        msgrcv(mq1,&buf,sizeof(buf.msg),1e6,0);
        printf("Process %d taken from ready queue\n", buf.msg);
        // buf.msg should contain the id
        int semid = semget(ftok("Process.c", buf.msg), 1, IPC_CREAT|0666);
        // send signal to the process
        pop.sem_num = vop.sem_num = 0;
        signall(semid);
        // scheduler blocks itself
        struct msgbuf buf2; 
        msgrcv(mq2,&buf2,sizeof(buf2.msg),0,0);
        // notification from MMU received
        if (buf2.mtype == 1) {
            // page fault handled, enqueing process back to ready queue
            struct msgbuf buff;
            buff.mtype = 1;
            buff.msg = buf2.msg;
            msgsnd(mq1,&buff,sizeof(buff.msg),0);
        }
        else if (buf2.mtype == 2) {
            numOfTerminatedProcesses++;
        }
    }
    signall(sem1);
    // can it end itself or we needa wait for master to kill????
    shmdt(shm_k_ptr);
    return 0;
}