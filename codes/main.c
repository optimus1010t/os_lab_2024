#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* The signal handler for the child process */
void childSigHandler ( int sig )
{
    if (sig == SIGUSR1) {
        printf("\n+++ Child : Received signal SIGUSR1 from parent...\n");
        sleep(1);
    } else if (sig == SIGUSR2) {
        printf("\n+++ Child : Received signal SIGUSR2 from parent...\n");
        sleep(5);
    }
}

int main ()
{   
    pid_t pid;
    if ((pid = fork()) == 0) {
        printf("\n+++ Child : I am the child process with PID = %d\n", getpid());
        int gpid;
        gpid = getpgid(getpid());
        printf("\n+++ Child : My process group ID is %d\n", gpid);
    }
    else{
        printf("\n+++ Parent : I am the parent process with PID = %d\n", getpid());
        int gpid;
        gpid = getpgid(getpid());
        printf("\n+++ Parent : My process group ID is %d\n", gpid);}
        exit(0);
}
