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


#include <stdio.h>
#include <stdlib.h>

#define ARGS 10

typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
} pageTableEntry;

typedef struct freeFrameList{
    int frameNumber;
    struct freeFrameList *next;
} freeFrameList;

int main(){
    srand(time(0));

    // taking user input
    int k,m,f;
    printf("Enter total number of processes(k): ");
    scanf("%d",&k);
    printf("Enter maximum number of pages required per process(m): ");
    scanf("%d",&m);
    printf("Enter total number of frames(f): ");
    scanf("%d",&f);

    // page tables (implemented as 1 dimensional array of size k*m)
    int keysm1=ftok("Master.c",'D');
    int sm1id=shmget(keysm1,k*m*sizeof(pageTableEntry),IPC_CREAT|0666);
    pageTableEntry *pageTables; // remember to detach and remove shared memory and free ????
    pageTables=(pageTableEntry*)shmat(sm1id,NULL,0);
    for(int i=0;i<k;i++){
        for(int j=0;j<m;j++){
            pageTables[m*i+j].valid = 0;
            pageTables[m*i+j].frameNumber = -1;
        }
    }

    // free frame list
    int keysm2=ftok("Master.c",'E');
    int sm2id=shmget(keysm2,sizeof(freeFrameList),IPC_CREAT|0666);
    freeFrameList *freeFrameListHead;
    freeFrameListHead=(freeFrameList*)shmat(sm2id,NULL,0);

    // process to page number mapping
    int numOfPagesReqd[k];

    for(int i=0;i<k;i++){
        numOfPagesReqd[i] = rand()%m + 1;
    }

    // mq1, mq2 and mq3
    int keymq1=ftok("Master.c",'A');
    int mq1id=msgget(keymq1,IPC_CREAT|0666);
    int keymq2=ftok("Master.c",'B');
    int mq2id=msgget(keymq2,IPC_CREAT|0666);
    int keymq3=ftok("Master.c",'C');
    int mq3id=msgget(keymq3,IPC_CREAT|0666);

    char sm1[10], sm2[10], mq1[10], mq2[10], mq3[10];
    memset(sm1,0,10); memset(sm2,0,10); memset(mq1,0,10); memset(mq2,0,10); memset(mq3,0,10);
    sprintf(sm1,"%d",sm1id);
    sprintf(sm2,"%d",sm2id);
    sprintf(mq1,"%d",mq1id);
    sprintf(mq2,"%d",mq2id);
    sprintf(mq3,"%d",mq3id);


    // passing ids as cmd line args

    // child process to execute scheduler
    if(!fork()){
        execlp("./Scheduler","Scheduler",mq1,mq2,NULL);
    }

    // child process to execute MMU
    if(!fork()){
        execlp("./MMU","MMU",mq2,mq3,sm1,sm2,NULL);
    }

    for(int i = 0; i < k; i++){
        usleep(250000);      
        //generate reference string
        int len = rand()%(8*numOfPagesReqd[i]+1)+2*numOfPagesReqd[i];
        char vec[len+4][ARGS];
        strcpy(vec[0],"Process");
        strcpy(vec[1],mq1);
        strcpy(vec[2],mq3);
        // sprintf(vec[3],"%d",len);
        // generate random reference string
        for(int j = 3; j < len; j++){
            int prob = rand()%100;
            if(prob < 20){
                prob = rand()%100;
                if(prob < 20) {
                    int illegal = rand()+numOfPagesReqd[i];
                    sprintf(vec[j],"%d",illegal);
                }
                else {
                    sprintf(vec[j],"%d",numOfPagesReqd[i]+rand()%(m-numOfPagesReqd[i]+1));  // one based indexing -> what logic ????
                    // sprintf(vec[j+1],"%d",rand()%numOfPagesReqd[i]+1);   // shouldnt this be the one ????
                } 
            }
            else{
                sprintf(vec[j],"%d",rand()%numOfPagesReqd[i]);
            }
        }
        for (int k = 0; k < ARGS; k++) vec[len+3][k]='\0';

        // child process to execute process
        if(!fork()){
            // execlp("./Process","Process",mq1,mq3,vec,NULL);
            execvp("./Process",vec);
        }
    }

    // wait until scheduler notifies that all processes are done


    // terminate scheduler and MMU
    kill(0,SIGTERM);  // probably works????

    // deallocate shared memory and message queues
    shmdt(pageTables);
    shmctl(sm1id,IPC_RMID,NULL);
    shmdt(freeFrameListHead);
    shmctl(sm2id,IPC_RMID,NULL);
    msgctl(mq1id,IPC_RMID,NULL);
    msgctl(mq2id,IPC_RMID,NULL);
    msgctl(mq3id,IPC_RMID,NULL);

    return 0;
}