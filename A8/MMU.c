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


typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
    int lastUsedAt;
} pageTableEntry;

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

void PageFaultHandler(int pageNumber, int pid, int mq2, int mq3, int *isFrameFree, pageTableEntry *pageTables, int *table_assgn, int k, int m, int f){
    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
    // find a free frame
    int frameNumber=-1;
    for(int i=0;i<f;i++){
        if(isFrameFree[i]==1){
            frameNumber=i;
            break;
        }
    }
    if (frameNumber!=-1) {
        printf("Free Frame: %d\n", frameNumber);
        int flag=pid;
        // send a message to the process
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
        isFrameFree[frameNumber]=0;

        // send a type 1 message to the scheduler
        struct msgbuf buf;
        buf.mtype = 1;
        buf.msg = pid;
        printf("MMU: mtype=%d, msg=%d\n", buf.mtype, buf.msg);
        msgsnd(mq2,&buf,sizeof(buf.msg),0);
    }
    // if no free frame is available
    else {
        printf("No Free Frame\n");
        int flag=0;
        // check if the set is empty
        for(int i=0;i<m;i++){
            if(pageTables[m*pid+i].valid){
                flag=1;
                break;
            }
        }
        // if set is empty, can't do anything, so sending message to scheduler to enqueue the process for later, is that correct????
        if(!flag){
            printf("Set is empty\n");
            struct msgbuf buf;
            buf.mtype = 1;
            buf.msg = pid;
            printf("MMU: mtype=%d, msg=%d\n", buf.mtype, buf.msg);
            msgsnd(mq2,&buf,sizeof(buf.msg),0);
        }
        else{
            // if set is not empty, replace the page with the highest time
            int page_to_replace = -1;
            int max = -1;
            for(int i=0;i<m;i++){
                if(i==pageNumber) continue;     // redundant, but still
                if(pageTables[m*flag+i].valid==1 && pageTables[m*flag+i].lastUsedAt > max){
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

            // send a type 1 message to the scheduler
            struct msgbuf buf;
            buf.mtype = 1;
            buf.msg = pid;
            printf("MMU: mtype=%d, msg=%d\n", buf.mtype, buf.msg);
            msgsnd(mq2,&buf,sizeof(buf.msg),0);
        }
    }
}

int main(int argc, char *argv[]){
    int fd=open("output.txt",O_WRONLY|O_CREAT|O_TRUNC,0666);
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

    int pageFaults[k];
    memset(pageFaults,0,sizeof(pageFaults));

    int invalidPageReferences[k];
    memset(invalidPageReferences,0,sizeof(invalidPageReferences));

    int maxPageindex[k];
    for(int i=0;i<k;i++) maxPageindex[i] = shm_m_f_table_ptr[i+2];

    int table_assgn[k];
    for(int i=0;i<k;i++) table_assgn[i] = -1;

    int mq2, mq3, sm1id, sm2id;
    mq2=atoi(argv[1]);
    mq3=atoi(argv[2]);
    sm1id=atoi(argv[3]);
    sm2id=atoi(argv[4]);
    printf("MMU: mq2=%d, mq3=%d\n", mq2, mq3);

    pageTableEntry *pageTables; // remember to detach and remove shared memory and free ????
    pageTables=(pageTableEntry*)shmat(sm1id,NULL,0);

    int *isFrameFree;
    isFrameFree=(int*)shmat(sm2id,NULL,0);

    while (1) {
        globaltime++;
        printf("Global Time: %ld\n", globaltime);
        struct msgbuf3 buf3;
        if(msgrcv(mq3,&buf3,sizeof(buf3.info),0,0)<0)
        {
            perror("MMU:msgrcv");
            exit(1);
        }
        printf("mtype: %ld\n", buf3.mtype);
        // if(buf3.mtype<=k){
        //     msgsnd(mq3,&buf3,sizeof(buf3.info),0);
        //     continue;
        // }
        int pageNumber = buf3.info.pageNumber;
        int pid = buf3.info.pid;
        int msg = buf3.info.msg;

        // output
        printf("Global Ordering: (%ld, %d, %d)\n", globaltime, pid, pageNumber);
        fflush(stdout);
        write(fd, "Global Ordering: (", 18);
        char temp[100];
        sprintf(temp, "%ld", globaltime);
        write(fd, temp, strlen(temp));
        write(fd, ", ", 2);
        sprintf(temp, "%d", pid);
        write(fd, temp, strlen(temp));
        write(fd, ", ", 2);
        sprintf(temp, "%d", pageNumber);
        write(fd, temp, strlen(temp));
        write(fd, ")\n", 2);
        
        // printf("process %d: ", pid);
        if (pageNumber == -9) {
            // add the frames in its page table to the free list
            int flag = pid;
            for (int i = 0; i < m; i++) {
                if (pageTables[m*flag+i].valid == 1) {
                    isFrameFree[pageTables[m*flag+i].frameNumber] = 1;
                }
            }
            struct msgbuf buf;
            buf.mtype = 2;
            buf.msg = pid;
            printf("MMU: mtype=%d, msg=%d\n", buf.mtype, buf.msg);
            msgsnd(mq2,&buf,sizeof(buf.msg),0);   
            printf("terminated\n");        
        }
        else {
            int flag = pid;
            // check if the page is already in the memory
            if(pageNumber > maxPageindex[flag]) {
                // illegal access
                struct msgbuf3 buf3;
                buf3.mtype = 2;
                buf3.info.pid = pid;
                buf3.info.pageNumber = pageNumber;
                buf3.info.msg = -2;
                // buf3.mtype = 1;
                // buf3.msg = -2;
                printf("Sending (%d, %d, %d)\n", pid, pageNumber, -2);
                msgsnd(mq3,&buf3,sizeof(buf3.info),0);
                struct msgbuf buf;
                buf.mtype = 2;
                buf.msg = pid;
                printf("MMU: mtype=%d, msg=%d\n", buf.mtype, buf.msg);
                msgsnd(mq2,&buf,sizeof(buf.msg),0);
                // printf("seg fault\n");
                printf("Invalid Page Reference: (%d, %d)\n", pid, pageNumber);
                fflush(stdout);
                write(fd, "Invalid Page Reference: (", 25);
                char temp[100];
                sprintf(temp, "%d", pid);
                write(fd, temp, strlen(temp));
                write(fd, ",", 1);
                sprintf(temp, "%d", pageNumber);
                write(fd, temp, strlen(temp));
                write(fd, ")\n", 2);
                invalidPageReferences[pid]++;
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
                struct msgbuf3 buf;
                buf.mtype=2;
                buf.info.msg=pageTables[m*flag+pageNumber].frameNumber;
                buf.info.pageNumber=pageTables[m*flag+pageNumber].frameNumber;
                buf.info.pid=pid;
                printf("Sending (%d, %d, %d)\n", pid, pageNumber, pageTables[m*flag+pageNumber].frameNumber);
                msgsnd(mq3,&buf,sizeof(buf.info),0);
                printf("assigned framenumber %d to page %d\n", buf.info.msg,pageNumber);
            }
            else {
                // page fault
                struct msgbuf3 buf;
                buf.mtype = 2;
                buf.info.pid = pid;
                buf.info.pageNumber = -1;
                buf.info.msg = -1;
                printf("Sending (%d, %d, %d)\n", pid, pageNumber, -1);
                msgsnd(mq3,&buf,sizeof(buf.info),0);
                printf("\tPage Fault Sequence: (%d,%d)\n", pid, pageNumber);
                fflush(stdout);
                write(fd, "\tPage Fault Sequence: (", 23);
                char temp[100];
                sprintf(temp, "%d", pid);
                write(fd, temp, strlen(temp));
                write(fd, ",", 1);
                sprintf(temp, "%d", pageNumber);
                write(fd, temp, strlen(temp));
                write(fd, ")\n", 2);
                pageFaults[pid]++;
                PageFaultHandler(pageNumber, pid, mq2, mq3, isFrameFree, pageTables, table_assgn, k, m, f);
            }
        }
    }
}