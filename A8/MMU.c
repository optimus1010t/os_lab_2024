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


typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
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
    // find a free frame
    freeFrameList *temp = freeFrameListHead;
    if (temp->frameNumber == -1) {
        // no free frame
        int flag = -1;
        for (int i = 0; i < k; i++) {
            if (table_assgn[i] == pid) {
                flag = i;
                break;
            }
        }
        if (flag == -1) return;
        // find a page to replace with valid bit 1  LRU ???? not implemented
        int page_to_replace = rand() % m;
        while (pageTables[m*flag+page_to_replace].valid == 0) {
            page_to_replace = rand() % m;
        }
        // assign the new page to the frame
        pageTables[m*flag+pageNumber-1].valid = 1;
        pageTables[m*flag+pageNumber-1].frameNumber = pageTables[m*flag+page_to_replace].frameNumber;
        // send a message to the process
        struct msgbuf buf;
        buf.mtype = 1;
        buf.msg = pid;
        msgsnd(mq2,&buf,sizeof(buf.msg),0);
    }
}

int main(int argc, char *argv[]){
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
        struct msgbuf3 buf3;
        msgrcv(mq3,&buf3,sizeof(buf3.info),0,0);
        int pageNumber = buf3.info.pageNumber;
        int pid = buf3.info.pid;
        int msg = buf3.info.msg;
        if (msg == -9) {
            // add the frames in its page table to the free list
            int flag = -1;
            for (int i = 0; i < k; i++) {
                if (table_assgn[i] == pid) {
                    flag = i;
                    break;
                }
            }
            if (flag == -1) continue;
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
        }
        else {
            int flag = -1, first_free = -1;
            for (int i = 0; i < k; i++) {
                if (table_assgn[i] == -1 && first_free == -1) {
                    first_free = i;
                }
                if (table_assgn[i] == pid) {
                    flag = i;
                    break;
                }
            }
            if (flag == -1) {
                if (first_free == -1) {
                    // no free space ???? send a message to the scheduler ????                    
                }
                else {
                    // assign the process to the first free space
                    table_assgn[first_free] = pid;
                    flag = first_free;
                }
            }
            // check if the page is already in the memory
            if (pageTables[m*flag+pageNumber-1].valid == 1) {
                // send a message to the process
                struct msgbuf buf;
                buf.mtype = 1;
                buf.msg = pageTables[m*flag+pageNumber-1].frameNumber;
                msgsnd(mq2,&buf,sizeof(buf.msg),0);
            }
            else {
                // check if the page is in range or not
                // page fault
                struct msgbuf buf;
                buf.mtype = 1;
                buf.msg = -1;
                msgsnd(mq2,&buf,sizeof(buf.msg),0);
                PageFaultHandler(pageNumber, pid, mq2, mq3, freeFrameListHead, pageTables, table_assgn, k, m, f);
            }
        }
    }
}