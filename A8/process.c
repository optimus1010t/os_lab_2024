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

struct msgbuf{
    long mtype;
    int pid;
};

int main(int argc, char *argv[]){
    int mq1, mq3;
    mq1=atoi(argv[1]);
    mq3=atoi(argv[2]);
    struct msgbuf buf;
    buf.mtype = 1;
    buf.pid = getpid();
    msgsnd(mq3,&buf,sizeof(buf),0);

}