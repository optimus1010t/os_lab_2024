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

#define ARGS 10

#define wait(s) semop(s, &pop, 1) 
#define signall(s) semop(s, &vop, 1)

typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
    int lastUsedAt;
} pageTableEntry;

typedef struct freeFrameList{
    int frameNumber;
    struct freeFrameList *next;
} freeFrameList;

int semq1, semq2, semq3;

int main(){
    // srand(time(0));

    struct sembuf pop, vop ;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    // taking user input
    int k,m,f;
    printf("Enter total number of processes(k): ");
    scanf("%d",&k);
    printf("Enter maximum number of pages required per process(m): ");
    scanf("%d",&m);
    printf("Enter total number of frames(f): ");
    scanf("%d",&f);

    // page tables (implemented as 1 dimensional array of size k*m)
    printf("Creating page tables\n");
    int keysm1=ftok("Master.c",'D');
    int sm1id=shmget(keysm1,k*m*sizeof(pageTableEntry),IPC_CREAT|0666);
    pageTableEntry *pageTables; // remember to detach and remove shared memory and free ????
    pageTables=(pageTableEntry*)shmat(sm1id,NULL,0);
    for(int i=0;i<k;i++) {
        for(int j=0;j<m;j++) {
            pageTables[m*i+j].valid = 0;
            pageTables[m*i+j].frameNumber = -1;
            pageTables[m*i+j].lastUsedAt = -1;
        }
    }
    printf("Page tables created\n");

    // free frame list
    int keysm2=ftok("Master.c",'E');
    int sm2id=shmget(keysm2,sizeof(freeFrameList),IPC_CREAT|0666);
    freeFrameList *freeFrameListHead;
    freeFrameListHead=(freeFrameList*)shmat(sm2id,NULL,0);

    // add all frames as free
    freeFrameListHead->frameNumber = -1;
    freeFrameListHead->next = NULL;

    // process to page number mapping
    int numOfPagesReqd[k];
    int totalNumOfPagesReqd = 0;
    int frameNumber = 0;

    for(int i=0;i<k;i++) {
        numOfPagesReqd[i] = rand()%m + 1;
        totalNumOfPagesReqd += numOfPagesReqd[i];
    }

    for(int i=0;i<k;i++){
        int toAllocate=(numOfPagesReqd[i]*f)/totalNumOfPagesReqd;
        for(int j=0;j<toAllocate;j++){
            pageTables[m*i+j].frameNumber = frameNumber;
            pageTables[m*i+j].valid = 1;
            frameNumber++;
        }
    }

    freeFrameList *temp = freeFrameListHead;
    for(int i=frameNumber;i<f;i++) {
        temp->next = (freeFrameList*)malloc(sizeof(freeFrameList));
        temp = temp->next;
        temp->frameNumber = i;
        temp->next = NULL;
    }

    // stores k
    int shm_k_id = shmget(ftok("MMU.c",'A'),sizeof(int),IPC_CREAT|0666);
    int *shm_k_ptr = (int*)shmat(shm_k_id,NULL,0);
    *shm_k_ptr = k;

    // stores m, f and numOfPagesReqd
    int shm_m_f_table_id = shmget(ftok("MMU.c",'B'),(k+2)*sizeof(int),IPC_CREAT|0666);
    int *shm_m_f_table_ptr = (int*)shmat(shm_m_f_table_id,NULL,0);
    shm_m_f_table_ptr[0] = m;
    shm_m_f_table_ptr[1] = f;
    for (int i = 0; i < k; i++) shm_m_f_table_ptr[i+2] = numOfPagesReqd[i];

    // mq1, mq2 and mq3
    int keymq1=ftok("Master.c",'A');
    int mq1id=msgget(keymq1,IPC_CREAT|0666);
    int keymq2=ftok("Master.c",'B');
    int mq2id=msgget(keymq2,IPC_CREAT|0666);
    int keymq3=ftok("Master.c",'C');
    int mq3id=msgget(keymq3,IPC_CREAT|0666);

    char sm1[10], sm2[10], mq1[10], mq2[10], mq3[10];
    memset(sm1,0,10); memset(sm2,0,10); memset(mq1,0,10); memset(mq2,0,10); memset(mq3,0,10);
    sprintf(sm1,"%d",keysm1);
    sprintf(sm2,"%d",keysm2);
    sprintf(mq1,"%d",keymq1);
    sprintf(mq2,"%d",keymq2);
    sprintf(mq3,"%d",keymq3);

    int keysem1=ftok("Master.c",'F');
    int sem1=semget(keysem1,1,IPC_CREAT|0666);
    semctl(sem1,0,SETVAL,0);

    int keysemq1=ftok("Master.c",'G');
    semq1=semget(keysemq1,1,IPC_CREAT|0666);
    semctl(semq1,0,SETVAL,0);
    
    int keysemq2=ftok("Master.c",'H');
    semq2=semget(keysemq2,1,IPC_CREAT|0666);
    semctl(semq2,0,SETVAL,0);

    int keysemq3=ftok("Master.c",'I');
    semq3=semget(keysemq3,1,IPC_CREAT|0666);
    semctl(semq3,0,SETVAL,0);


    // passing keyss as cmd line args

    // child process to execute scheduler
    if(fork()==0) {
        execlp("./sched","./sched",mq1,mq2,NULL);
        perror("error1\n");
    }

    // child process to execute MMU
    if(fork()==0) {
        execlp("./MMU","./MMU",mq2,mq3,sm1,sm2,NULL);
        perror("error2\n");
    }

    for(int i = 0; i < k; i++){
        printf("creating process %d\n",i);
        usleep(250000);      
        //generate reference string
        int len = rand()%(8*numOfPagesReqd[i]+1)+2*numOfPagesReqd[i];
        printf("Reference string of length %d\n", len);
        char *vec[len+5];
        for(int j=0;j<len+5;j++) vec[j] = (char*)malloc(ARGS*sizeof(char));
        strcpy(vec[0],"./Process");
        strcpy(vec[1],mq1);
        strcpy(vec[2],mq3);
        sprintf(vec[3],"%d\0",i);
        printf("vec[3]: %s\n",vec[3]);
        // generate random reference string
        for(int j = 4; j < len+4; j++){
            int prob = rand()%100;
            if(prob < 20){
                prob = rand()%100;
                if(prob < 20) {
                    int illegal = rand()+m;
                    sprintf(vec[j],"%d\0",illegal);
                }
                else {
                    int pg=rand()%numOfPagesReqd[i];
                    sprintf(vec[j],"%d\0",pg);
                } 
            }
            else {
                sprintf(vec[j],"%d\0",rand()%numOfPagesReqd[i]);
            }
            printf("%s ",vec[j]);
        }
        printf("\n");
        vec[len+4]=NULL;

        // child process to execute process
        if(fork()==0) {
            execvp("./Process",vec);
            perror("error3\n");
        }
    }

    // wait until scheduler notifies that all processes are done
    printf("here\n");
    wait(sem1);


    // terminate scheduler and MMU
    kill(0,SIGTERM);

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