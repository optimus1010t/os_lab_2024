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

long int globaltime=0;

int semq1, semq2, semq3;


typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
    int lastUsedAt;
} pageTableEntry;

typedef struct freeFrameList{
    int frameNumber;
    struct freeFrameList *next;
} freeFrameList;

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

void PageFaultHandler(int pageNumber, int pid, int mq2, int mq3, freeFrameList *freeFrameListHead, pageTableEntry *pageTables, int *table_assgn, int k, int m, int f){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    // find a free frame
    freeFrameList *temp = freeFrameListHead->next;
    if (temp!=NULL) {
        int flag=pid;
        // send a message to the process
        int frameNumber = temp->frameNumber;
        for(int i=0;i<m;i++){
            // the case when miss occurs and set is not full, make new block time 0, and all valid others time incremented by 1
            if(pageTables[m*flag+i].valid == 0){
                pageTables[m*flag+i].valid = 1;
                pageTables[m*flag+i].frameNumber = frameNumber;
                pageTables[m*flag+i].lastUsedAt = 0;
                for(int j=0;j<m;j++){
                    if(pageTables[m*flag+j].valid==1 && j!=i){
                        pageTables[m*flag+j].lastUsedAt++;
                    }
                }
                break;
            }
        }
        // remove the frame from the free list
        freeFrameListHead->next = temp->next;
        free(temp);
        // send a message to the process
        struct msgbuf buf;
        buf.mtype = 1;
        buf.msg = pid;
        msgsnd(mq2,&buf,sizeof(buf.msg),0);
        wait(semq2);
    }
    else {
        int flag=0;
        for(int i=0;i<m;i++){
            if(pageTables[m*pid+i].valid){
                flag=1;
                break;
            }
        }
        if(!flag){
            // send a message to the MMU
            struct msgbuf buf;
            buf.mtype = 1;
            buf.msg = pid;
            msgsnd(mq2,&buf,sizeof(buf.msg),0);
            wait(semq2);
        }
        else{
            // if set is full, replace the page with the highest time
            int page_to_replace = 0;
            int max = pageTables[m*flag].lastUsedAt;
            for(int i=1;i<m;i++){
                if(pageTables[m*flag+i].lastUsedAt > max){
                    max = pageTables[m*flag+i].lastUsedAt;
                    page_to_replace = i;
                }
            }
            // assign the new page to the frame
            pageTables[m*flag+pageNumber].valid = 1;
            pageTables[m*flag+pageNumber].frameNumber = pageTables[m*flag+page_to_replace].frameNumber;
            for(int i=0;i<m;i++){
                if(pageTables[m*flag+i].valid==1 && i!=pageNumber){
                    pageTables[m*flag+i].lastUsedAt++;
                }
            }
            pageTables[m*flag+pageNumber].lastUsedAt = 0;
            pageTables[m*flag+page_to_replace].valid = 0;
            pageTables[m*flag+page_to_replace].frameNumber = -1;
            pageTables[m*flag+page_to_replace].lastUsedAt = -1;

            // send a message to the process
            struct msgbuf buf;
            buf.mtype = 1;
            buf.msg = pid;
            msgsnd(mq2,&buf,sizeof(buf.msg),0);
            wait(semq2);
        }
    }
}

int main(int argc, char *argv[]){
    semq1 = semget(ftok("Master.c",'G'), 1, IPC_CREAT|0666);
    semq2 = semget(ftok("Master.c",'H'), 1, IPC_CREAT|0666);
    semq3 = semget(ftok("Master.c",'I'), 1, IPC_CREAT|0666);

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    int shm_k_id = shmget(ftok("MMU.c",'A'),sizeof(int),IPC_CREAT|0666);
    int *shm_k_ptr = (int*)shmat(shm_k_id,NULL,0);
    int k = *shm_k_ptr;

    int shm_m_f_table_id = shmget(ftok("MMU.c",'B'),(k+2)*sizeof(int),IPC_CREAT|0666);
    int *shm_m_f_table_ptr = (int*)shmat(shm_m_f_table_id,NULL,0);
    int m = shm_m_f_table_ptr[0];
    int f = shm_m_f_table_ptr[1];

    int maxPageindex[k];
    for(int i=0;i<k;i++) maxPageindex[i] = shm_m_f_table_ptr[i+2];

    int table_assgn[k];
    for(int i=0;i<k;i++) table_assgn[i] = -1;

    int keymq2, keymq3, keysm1, keysm2;  
    keymq2=atoi(argv[1]);
    keymq3=atoi(argv[2]);
    keysm1=atoi(argv[3]);
    keysm2=atoi(argv[4]);
    int mq2, mq3, sm1id, sm2id;
    mq2 = msgget(keymq2, IPC_CREAT|0666);
    mq3 = msgget(keymq3, IPC_CREAT|0666);
    sm1id = shmget(keysm1,k*m*sizeof(pageTableEntry),IPC_CREAT|0666);
    pageTableEntry *pageTables; // remember to detach and remove shared memory and free ????
    pageTables=(pageTableEntry*)shmat(sm1id,NULL,0);

    sm2id = shmget(keysm2,sizeof(freeFrameList),IPC_CREAT|0666);
    freeFrameList *freeFrameListHead;
    freeFrameListHead=(freeFrameList*)shmat(sm2id,NULL,0);

    while (1) {
        globaltime++;
        struct msgbuf3 buf3;
        msgrcv(mq3,&buf3,sizeof(buf3.info),0,0);
        signall(semq3);
        int pageNumber = buf3.info.pageNumber;
        int pid = buf3.info.pid;
        int msg = buf3.info.msg;
        printf("Global Ordering: (%ld, %d, %d)\n", globaltime, pid, pageNumber);
        printf("process %d: ", pid);
        if (msg == -9) {
            // add the frames in its page table to the free list
            int flag = pid;
            freeFrameList *temp = freeFrameListHead;
            while (temp->next != NULL) temp = temp->next;
            for (int i = 0; i < m; i++) {
                if (pageTables[m*flag+i].valid == 1) {
                    temp->next = (freeFrameList*)malloc(sizeof(freeFrameList));
                    temp = temp->next;
                    temp->frameNumber = pageTables[m*flag+i].frameNumber;
                    temp->next = NULL;
                }
            }
            struct msgbuf buf;
            buf.mtype = 2;
            buf.msg = pid;
            msgsnd(mq2,&buf,sizeof(buf.msg),0);   
            wait(semq2);
            printf("terminated\n");        
        }
        else {
            int flag = pid;
            // check if the page is already in the memory
            if(pageNumber > maxPageindex[flag]) {
                // illegal access
                struct msgbuf3 buf3;
                buf3.mtype = 1;
                buf3.info.pid = pid;
                buf3.info.pageNumber = pageNumber;
                buf3.info.msg = -2;
                // buf3.mtype = 1;
                // buf3.msg = -2;
                msgsnd(mq3,&buf3,sizeof(buf3.info),0);
                wait(semq3);
                struct msgbuf buf;
                buf.mtype = 2;
                buf.msg = pid;
                msgsnd(mq2,&buf,sizeof(buf.msg),0);
                wait(semq2);
                printf("seg fault\n");
                continue;
            }
            if (pageTables[m*flag+pageNumber].valid == 1) {
                // hit occurs, update the last used time, increment all other times originally lesser
                int cmp = pageTables[m*flag+pageNumber].lastUsedAt;
                for(int i=0;i<m;i++){
                    if(pageTables[m*flag+i].valid==1 && pageTables[m*flag+i].lastUsedAt < cmp){
                        pageTables[m*flag+i].lastUsedAt++;
                    }
                }
                pageTables[m*flag+pageNumber].lastUsedAt = 0;

                // send a message to the process
                struct msgbuf buf;
                buf.mtype = 1;
                buf.msg = pageTables[m*flag+pageNumber].frameNumber;
                msgsnd(mq2,&buf,sizeof(buf.msg),0);
                wait(semq2);
                printf("assigned framenumber %d to page %d\n", buf.msg,pageNumber);
            }
            else {
                // page fault
                struct msgbuf3 buf;
                buf.mtype = 1;
                buf.info.pid = pid;
                buf.info.pageNumber = -1;
                buf.info.msg = -1;
                msgsnd(mq3,&buf,sizeof(buf.info),0);
                wait(semq3);
                printf("page fault @ %d\n", pageNumber);
                PageFaultHandler(pageNumber, pid, mq2, mq3, freeFrameListHead, pageTables, table_assgn, k, m, f);
            }
        }
    }
}