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

#define ARGS 20

#define wait(s) semop(s, &pop, 1) 
#define signall(s) semop(s, &vop, 1)

typedef struct pageTableEntry{ // page table entry
    int frameNumber;
    int valid;
    int lastUsedAt;
} pageTableEntry;

int mq1id, mq2id, mq3id, sm1id, sm2id;
int sem1;
int shm_k_id, shm_m_f_table_id;
int *shm_k_ptr, *shm_m_f_table_ptr;
pageTableEntry *pageTables;
int *isFrameFree;

void sighandler(int signum){
    if(signum == SIGINT){
        printf("Master process interrupted\n");
        shmdt(pageTables);
        shmctl(sm1id,IPC_RMID,NULL);
        shmdt(isFrameFree);
        shmctl(sm2id,IPC_RMID,NULL);
        msgctl(mq1id,IPC_RMID,NULL);
        msgctl(mq2id,IPC_RMID,NULL);
        msgctl(mq3id,IPC_RMID,NULL);
        semctl(sem1,0,IPC_RMID,0);
        shmdt(shm_k_ptr);
        shmdt(shm_m_f_table_ptr);
        shmctl(shm_k_id,IPC_RMID,NULL);
        shmctl(shm_m_f_table_id,IPC_RMID,NULL);
        exit(0);
    }
}

int main(){
    srand(time(0));
    signal(SIGINT,sighandler);

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
    int keysm1=ftok("Master.c",'D');
    sm1id=shmget(keysm1,k*m*sizeof(pageTableEntry),IPC_CREAT|0666);
    pageTables=(pageTableEntry*)shmat(sm1id,NULL,0);
    for(int i=0;i<k;i++) {
        for(int j=0;j<m;j++) {
            pageTables[m*i+j].valid = 0;
            pageTables[m*i+j].frameNumber = -1;
            pageTables[m*i+j].lastUsedAt = -1;
        }
    }

    // free frame list
    int keysm2=ftok("Master.c",'E');
    sm2id=shmget(keysm2,sizeof(int)*(f),IPC_CREAT|0666);
    isFrameFree=(int*)shmat(sm2id,NULL,0);

    // process to page number mapping
    int numOfPagesReqd[k];
    int totalNumOfPagesReqd = 0;
    int frameNumber = 0;

    // pids of scheduler and mmu
    int pidSched, pidMMU;

    for(int i=0;i<k;i++) {
        numOfPagesReqd[i] = rand()%m + 1;
        totalNumOfPagesReqd += numOfPagesReqd[i];
    }

    for(int i=0;i<k;i++){
        int lu=0;
        int toAllocate=(numOfPagesReqd[i]*f)/totalNumOfPagesReqd;
        for(int j=0;(frameNumber<f)&&(j<((numOfPagesReqd[i]<toAllocate)?numOfPagesReqd[i]:toAllocate));j++){
            pageTables[m*i+j].frameNumber = frameNumber;
            pageTables[m*i+j].valid = 1;
            pageTables[m*i+j].lastUsedAt=lu;
            lu++;
            isFrameFree[frameNumber]=0;
            frameNumber++;
        }
    }

    for(int i=frameNumber;i<f;i++) {
        isFrameFree[i]=1;
    }

    // stores k
    shm_k_id = shmget(ftok("MMU.c",'A'),sizeof(int),IPC_CREAT|0666);
    shm_k_ptr = (int*)shmat(shm_k_id,NULL,0);
    *shm_k_ptr = k;

    // stores m, f and numOfPagesReqd
    shm_m_f_table_id = shmget(ftok("MMU.c",'B'),(k+2)*sizeof(int),IPC_CREAT|0666);
    shm_m_f_table_ptr = (int*)shmat(shm_m_f_table_id,NULL,0);
    shm_m_f_table_ptr[0] = m;
    shm_m_f_table_ptr[1] = f;
    for (int i = 0; i < k; i++) shm_m_f_table_ptr[i+2] = numOfPagesReqd[i];

    // mq1, mq2 and mq3
    int keymq1=ftok("Master.c",'A');
    mq1id=msgget(keymq1,IPC_CREAT|0666);
    int keymq2=ftok("Master.c",'B');
    mq2id=msgget(keymq2,IPC_CREAT|0666);
    int keymq3=ftok("Master.c",'C');
    mq3id=msgget(keymq3,IPC_CREAT|0666);

    char sm1[ARGS], sm2[ARGS], mq1[ARGS], mq2[ARGS], mq3[ARGS];
    memset(sm1,0,ARGS); memset(sm2,0,ARGS); memset(mq1,0,ARGS); memset(mq2,0,ARGS); memset(mq3,0,ARGS);
    sprintf(sm1,"%d",sm1id);
    sprintf(sm2,"%d",sm2id);
    sprintf(mq1,"%d",mq1id);
    sprintf(mq2,"%d",mq2id);
    sprintf(mq3,"%d",mq3id);

    int keysem1=ftok("Master.c",'F');
    sem1=semget(keysem1,1,IPC_CREAT|0666);
    semctl(sem1,0,SETVAL,0);

    // passing keyss as cmd line args

    // child process to execute scheduler
    if((pidSched=fork())==0) {
        execlp("./sched","./sched",mq1,mq2,NULL);
        perror("error1\n");
    }

    // child process to execute MMU
    if((pidMMU=fork())==0) {
        execlp("xterm", "xterm", "-T", "MMU", "-e", "./MMU",mq2,mq3,sm1,sm2,NULL);
        perror("error2\n");
    }

    for(int i = 0; i < k; i++){
        usleep(250000);      
        //generate reference string
        int len = rand()%(8*numOfPagesReqd[i]+1)+2*numOfPagesReqd[i];
        char *vec[len+5];
        for(int j=0;j<len+5;j++) vec[j] = (char*)malloc(ARGS*sizeof(char));
        strcpy(vec[0],"./Process");
        strcpy(vec[1],mq1);
        strcpy(vec[2],mq3);
        sprintf(vec[3],"%d\0",i);
        // generate random reference string
        for(int j = 4; j < len+4; j++){
            int prob = rand()%100;
            if(prob < 20){
                prob = rand()%100;
                if(prob < 20) {
                    // illegal
                    int illegal = rand()+numOfPagesReqd[i];
                    sprintf(vec[j],"%d\0",illegal);
                }
                else {
                    // invalid but legal
                    int toAllocate=(numOfPagesReqd[i]*f)/totalNumOfPagesReqd;
                    int min2 = (numOfPagesReqd[i]<toAllocate)?numOfPagesReqd[i]:toAllocate;
                    if (min2 == numOfPagesReqd[i]) {
                        j--;
                        continue;
                    }
                    int pg=min2+rand()%(numOfPagesReqd[i]-min2);
                    sprintf(vec[j],"%d\0",pg);
                } 
            }
            else {
                // legal
                sprintf(vec[j],"%d\0",rand()%numOfPagesReqd[i]);
            }
        }
        vec[len+4]=NULL;

        // child process to execute process
        if(fork()==0) {
            execvp("./Process",vec);
            perror("error3\n");
        }
    }

    // wait until scheduler notifies that all processes are done
    wait(sem1);


    // terminate scheduler and MMU
    kill(pidSched,SIGINT);
    kill(pidMMU,SIGINT);

    // deallocate shared memory and message queues
    shmdt(pageTables);
    shmctl(sm1id,IPC_RMID,NULL);
    shmdt(isFrameFree);
    shmctl(sm2id,IPC_RMID,NULL);
    msgctl(mq1id,IPC_RMID,NULL);
    msgctl(mq2id,IPC_RMID,NULL);
    msgctl(mq3id,IPC_RMID,NULL);
    semctl(sem1,0,IPC_RMID);
    shmdt(shm_k_ptr);
    shmdt(shm_m_f_table_ptr);
    shmctl(shm_k_id,IPC_RMID,NULL);
    shmctl(shm_m_f_table_id,IPC_RMID,NULL);

    return 0;
}