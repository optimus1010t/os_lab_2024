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

struct msgbuf3_r{
    long mtype;
    int msg;
};


struct msgbuf1{
    long mtype;
    int pid;
};

struct msgbuf3{
    long mtype;
    struct page_info{
        int pid;
        int pageNumber;
        int msg;
    } page_info;
};

int main(int argc, char *argv[]){
    int semid = semget(ftok("Process.c", getpid()), 2, IPC_CREAT|0666);
    for (int i = 0; i < 2; i++) semctl(semid, i, SETVAL, 0);
    int mq1, mq3;
    mq1=atoi(argv[1]);
    mq3=atoi(argv[2]);
    struct msgbuf1 buf1;
    buf1.mtype = 1;
    buf1.pid = getpid();
    msgsnd(mq3,&buf1,sizeof(buf1.pid),0);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    wait(semid);
    int len = argc - 3;

    for (int i = 0; i < len; i++){
        // extract the number from the argument
        int num = atoi(argv[i+3]);
        struct msgbuf3 buf3;
        buf3.mtype = 1;
        buf3.page_info.pid = getpid();
        buf3.page_info.pageNumber = num;
        buf3.page_info.msg = 0;
        msgsnd(mq3,&buf3,sizeof(buf3.page_info),0);

        struct msgbuf3_r buf3_r;
        msgrecv(mq3,&buf3_r,sizeof(buf3_r.msg),0);

        if (buf3_r.msg == -1) {
            pop.sem_num = vop.sem_num = 1;
            signall(semid);  // scheduler will go on to the next process
            pop.sem_num = vop.sem_num = 0;
            wait(semid);
        }
        else if (buf3_r.msg == -2) {
            // terminate itself
            exit(0);
        }
    }
    struct msgbuf3 buf3;
    buf3.mtype = 1;
    buf3.page_info.pid = getpid();
    buf3.page_info.pageNumber = -1;
    buf3.page_info.msg = -9;
    msgsnd(mq3,&buf3,sizeof(buf3.page_info),0);
    exit(0);
    
}