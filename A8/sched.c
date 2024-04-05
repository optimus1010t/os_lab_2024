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

struct msgbuf2 {
    long mtype;
    struct info{
        int pid;
        int msg;
    } info;
};

int main(int argc, char *argv[]){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    int mq1, mq2;
    mq1=atoi(argv[1]);
    mq2=atoi(argv[2]);
    struct msgbuf buf;
    while(1){
        msgrcv(mq1,&buf,sizeof(buf),0,0);
        int semid = semget(ftok("Process.c", buf.msg), 2, IPC_CREAT|0666);
        // send signal to the process
        pop.sem_num = vop.sem_num = 0;
        signall(semid);
        // scheduler blocks itself
        struct msgbuf2 buf2; 
        msgrcv(mq2,&buf2,sizeof(buf2),0,0);
        // notification from MMU received
        if (buf2.info.msg == 1) {
            struct msgbuf buff;
            buff.mtype = 1;
            buff.msg = buf2.info.pid;
            msgsnd(mq1,&buff,sizeof(buff.msg),0);
        }
        else if (buf2.info.msg == 2) {
            continue;
        }
    }
}