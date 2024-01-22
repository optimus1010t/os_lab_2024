#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int pid = -1;
int c_pids[11];
char c_jobs[11];
int c_status[11];
void SigHandler_INT ( int sig )
{
    if (pid != -1) {
        kill(pid, SIGKILL);
        // - find the ondex in c_pids with value as pid
        int i = 0;
        while (c_pids[i] != pid) {
            i++;
        }
        c_status[i] = 2;
        printf("\n");
    }
    else printf("\nmgr> ");   
}

void SigHandler_TSTP ( int sig )
{
    if (pid != -1 ) 
    {
        kill(pid, SIGTSTP);
        int i = 0;
        while (c_pids[i] != pid) {
            i++;
        }
        c_status[i] = 3;
        printf("\n");
    }
    else printf("\nmgr> ");
    
}

void SigHandler_CONT ( int sig )
{
    if (pid != -1) kill(pid, SIGCONT);
    printf("\n");
}

int main ()
{
    srand((unsigned int)time(NULL));
    int count = 0;
    char ch; 
    

    // 1 : Finished
    // 2 : Terminated
    // 3 : Suspended
    // 4 : Killed
    // 5 : Self

    for (int i = 0; i < 11; i++) {
        c_pids[i] = -1;
        c_jobs[i] = '\0';
        c_status[i] = 0;
    }

    c_pids[0] = getpid();
    c_jobs[0] = '#';  // Manager
    c_status[0] = 5;  // Self    

    signal(SIGINT, SigHandler_INT);
    signal(SIGTSTP, SigHandler_TSTP);

    while (1) {
        if (ch != '\n') printf("mgr> ");
        ch = getchar();
        char c;
        fflush(stdin);
        switch(ch) {
            case 'c' : 
                // Continue a suspended job
                int count_suspend = 0;
                for (int i = 0; i < 11; i++) {
                    if (c_status[i] == 3) {
                        count_suspend++;
                    }
                }
                if (count_suspend == 0) {
                    printf("No suspended jobs\n");
                    break;
                }
                printf("Suspended jobs : ");
                for (int i = 0; i < 11; i++) {
                    if (c_status[i] == 3) {
                        count_suspend--;
                        printf("%d", i);
                        if (count_suspend != 0) printf(", ");
                    }
                }
                printf(" (Pick One): ");
                int n;
                scanf("%d", &n);
                // check if the job is suspended
                if (c_status[n] != 3) {
                    printf("Job not suspended\n");
                    break;
                }
                pid = c_pids[n];
                kill(pid, SIGCONT);
                waitpid(pid, NULL, WUNTRACED);
                c_status[n] = 1;
                pid = -1;
                break;

            case 'k' :
                int count_suspend1 = 0;
                for (int i = 0; i < 11; i++) {
                    if (c_status[i] == 3) {
                        count_suspend1++;
                    }
                }
                if (count_suspend1 == 0) {
                    printf("No suspended jobs\n");
                    break;
                }
                printf("Suspended jobs : ");
                for (int i = 0; i < 11; i++) {
                    if (c_status[i] == 3) {
                        count_suspend1--;
                        printf("%d", i);
                        if (count_suspend1 != 0) printf(", ");
                    }
                }
                printf(" (Pick One): ");
                int nj;
                scanf("%d", &nj);
                // check if the job is suspended
                if (c_status[nj] != 3) {
                    printf("Job not suspended\n");
                    break;
                }
                pid = c_pids[nj];
                kill(pid, SIGKILL);
                c_status[nj] = 4;
                pid = -1;
                break;
            
            case 'h' : 
                printf("\tCommand\t:\tAction\n");
                printf("\tc\t:\tContinue a suspended job\n"); 
                printf("\th\t:\tPrint this help message\n");
                printf("\tk\t:\tKill a suspended job\n");
                printf("\tp\t:\tPrint the process table\n");
                printf("\tq\t:\tQuit the program\n");
                printf("\tr\t:\tRun a new job\n");
                break;
            
            case 'r' :
                if (count == 10) {
                    printf("Process table is full. Quiting...\n");
                    exit(0);
                }
                char n_job = (char)('A' + rand() % 26);
                int i = 0;
                while (c_jobs[i] != '\0') {
                    if (c_jobs[i] == n_job) {
                        n_job = (char)('A' + rand() % 26);
                        i = 0;
                    } else {
                        i++;
                    }
                }
                c_jobs[i] = n_job;
                count++;
                if ((pid = fork()) == 0) {
                    setpgid (0, 0);
                    char spacey[2];
                    spacey[0] = n_job; spacey[1] = '\0';
                    char *args[] = {"./job", spacey, NULL};
                    execvp(args[0], args);
                    pid = -1;
                    exit(0);
                }
                else {
                    c_pids[i] = pid;
                    waitpid(pid, NULL, WUNTRACED);
                    pid = -1;
                }
                break;
            
            case 'p' :
                printf("\tNO\tPID\tPGID\tSTATUS\t\t\tNAME\n");
                int j = 0;
                while (c_pids[j] != -1){
                    printf("\t%d\t%d\t%d\t", j, c_pids[j], c_pids[j]);                    
                    if (c_status[j]==5){
                        printf("SELF\t\t\t");
                    } else
                    if (c_status[j]==2){
                        printf("TERMINATED\t\t");
                    } else
                    if (c_status[j]==3){
                        printf("SUSPENDED\t\t");
                    } else
                    if (c_status[j]==4){
                        printf("KILLED\t\t\t");
                    }
                    else {                        
                        printf("FINISHED\t\t");
                    }
                    if (j == 0) printf("mgr\n");
                    else printf("job %c\n", c_jobs[j]);
                    j++;
                }
                
                break;
            case 'q' : 
                printf("Exiting...\n");
                exit(0);
                break;

            case '\n' :
                break;

            default : 
                printf("Not a valid command, enter h for help and print the command palette\n");
                break;
        }
    }
    exit(0);
}
