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

struct msgbuf3 {  // standard for sending messages to the MMU
    long mtype;
    struct info{
        int pid;
        int pageNumber;
        int msg;
    } info;
};

int main(int argc, char *argv[]){
    int id = atoi(argv[3]);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    
    int semid = semget(ftok("Process.c", id+1), 1, IPC_CREAT|0666);
    semctl(semid, 0, SETVAL, 0);
    int mq1, mq3;
    mq1=atoi(argv[1]);
    mq3=atoi(argv[2]);

    struct msgbuf buf;
    buf.mtype = 1;
    buf.msg = id;
    msgsnd(mq1,&buf,sizeof(buf.msg),0);

    wait(semid);
    fflush(stdout);
    int len = argc - 4;

    for (int i = 0; i < len; i++){
        // extract the number from the argument
        int num = atoi(argv[i+4]);
        fflush(stdout);
        struct msgbuf3 buf3;
        // process sends message to MMU ONLY with type 1
        buf3.mtype = 1;
        buf3.info.pid = id;
        buf3.info.pageNumber = num;
        buf3.info.msg = 0;
        fflush(stdout);
        if(num==-1){
            sleep(10);
        }
        msgsnd(mq3,&buf3,sizeof(buf3.info),0);

        struct msgbuf3 buf3_r;
        fflush(stdout);
        msgrcv(mq3,&buf3_r,sizeof(buf3_r.info),2,0);
        fflush(stdout);

        if (buf3_r.info.msg == -1) {
            // pop.sem_num = vop.sem_num = 1;
            // signall(semid);  // scheduler will go on to the next process
            pop.sem_num = vop.sem_num = 0;
            wait(semid);
            i--;
        }
        else if (buf3_r.info.msg == -2) {
            // terminate itself
            semctl(semid, 0, IPC_RMID, 0);
            exit(0);
        }
    }
    struct msgbuf3 buf3;
    buf3.mtype = 1;
    buf3.info.pid = id;
    buf3.info.pageNumber = -9;
    buf3.info.msg = -9;
    msgsnd(mq3,&buf3,sizeof(buf3.info),0);
    semctl(semid, 0, IPC_RMID, 0);
    exit(0);    
}