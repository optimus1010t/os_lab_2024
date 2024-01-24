#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define MAX_WORDS 100  // Maximum number of words
#define MAX_WORD_LEN 50 // Maximum length of a word

int main (int argc, char* argv[])
{
    if (argc == 1) {
    int child1, child2;
    int p1[2];
    int p2[2];
    if (pipe(p1) < 0) {
        perror("pipe failed");
        exit(1);
    }
    if (pipe(p2) < 0) {
        perror("pipe failed");
        exit(1);
    }
    printf("+++ CSE in supervisor mode: Started\n");
    printf("+++ CSE in supervisor mode: pfd = [%d %d]\n", p1[0], p1[1]);
    int pid;
    int pid_c1, pid_c2;
    if ((pid = fork()) != 0) {
        printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
        pid_c1 = pid;
        char p10[10], p11[10], p20[10], p21[10];
        sprintf(p10, "%d", p1[0]);
        sprintf(p11, "%d", p1[1]);
        sprintf(p20, "%d", p2[0]);
        sprintf(p21, "%d", p2[1]);
        execlp("xterm", "xterm", "-T", "First Child", "-e", "./CSE","0", p10, p11, p20, p21, NULL);
    }
    else if (pid = fork() != 0) {
        printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");
        pid_c2 = pid;
        char p10[10], p11[10], p20[10], p21[10];
        sprintf(p10, "%d", p1[0]);
        sprintf(p11, "%d", p1[1]);
        sprintf(p20, "%d", p2[0]);
        sprintf(p21, "%d", p2[1]);
        execlp("xterm", "xterm", "-T", "Second Child", "-e", "./CSE","1", p10, p11, p20, p21, NULL);
    }
    }
    else {
        int a1 = atoi(argv[1]);
        int a2 = atoi(argv[2]);
        int a3 = atoi(argv[3]);
        int a4 = atoi(argv[4]);
        int a5 = atoi(argv[5]);
        int swap = 0;
        
        if (a1 == 0){
            int out_b, in_b;
            out_b = dup(1);
            in_b = dup(0);
            close(1);
            dup(a3);            
            while(1){
                if (swap == 0) {
                    char buffer[1024];
                    write(2,"Enter command> ", 15);
                    fgets(buffer,1024, stdin);
                    printf("%s",buffer);
                    fflush(stdout);
                    if (strcmp(buffer,"exit\n") == 0) {
                        exit(0);
                    }
                    if (strcmp(buffer,"swaprole\n") == 0) {
                        swap = 1;                        
                        close(0);
                        dup(a4);
                        close(1);
                        dup(out_b);
                        for (int i = 0; i < 1024; i++) buffer[i] = '\0';
                        continue;
                    }
                    fflush(stdout);

                }
                if (swap == 1) {
                    char buffer[1024];
                    write(2,"Waiting for command> ",21);
                    fgets(buffer,1024, stdin);
                    printf("%s", buffer);
                    fflush(stdout);
                    if (strcmp(buffer,"exit\n") == 0){
                        close(a3);
                        exit(0);
                    }
                    if (strcmp(buffer,"swaprole\n") == 0) {
                        swap = 0;
                        close(1);
                        dup(a3);
                        close(0);
                        dup(in_b);
                        for (int i = 0; i < 1024; i++) buffer[i] = '\0'; 
                        continue;
                    }
                    static char* words[MAX_WORDS + 1];  // Static array of string pointers
                    int wordCount = 0;
                    if (buffer[strlen(buffer)-1]=='\n') buffer[strlen(buffer)-1] = '\0';
                    // Make a copy of the input line to tokenize
                    char* lineCopy = strdup(buffer);
                    if (lineCopy == NULL) {
                        perror("strdup failed");
                    }
                    // Tokenize the string and store words
                    char* word = strtok(lineCopy, " ");
                    while (word != NULL && wordCount < MAX_WORDS) {
                        words[wordCount] = strdup(word); // Duplicate the word
                        if (words[wordCount] == NULL) {
                            perror("strdup failed");
                            free(lineCopy);
                        }
                        wordCount++;
                        word = strtok(NULL, " ");
                    }
                    words[wordCount] = NULL;  // Null-terminate the array of strings
                    int pid;
                    free(lineCopy);
                    if (pid=fork() == 0) {
                        close(0);
                        dup(in_b);
                        int check = execvp(words[0],words);
                        if (check == -1) {
                            printf("*** Unable to execute command\n");
                        }
                        exit(0);
                    }
                    else waitpid(pid, NULL, WUNTRACED);
                    fflush(stdout);
                    // printf("\n");
                }
            }
        }
        else {
            int out_b, in_b;
            out_b = dup(1);
            in_b = dup(0);
            close(0);
            dup(a2);
            while(1){
                if (swap == 0) {
                    char buffer[1024];
                    write(2,"Waiting for command> ",21);
                    fgets(buffer,1024, stdin);
                    printf("%s", buffer);
                    fflush(stdout);
                    if (strcmp(buffer,"exit\n") == 0){
                        exit(0);
                    }
                    if (strcmp(buffer,"swaprole\n") == 0) {
                        swap = 1;
                        close(1);
                        dup(a5);
                        close(0);
                        dup(in_b);
                        for (int i = 0; i < 1024; i++) buffer[i] = '\0';
                        continue;
                    }
                    static char* words[MAX_WORDS + 1];  // Static array of string pointers
                    int wordCount = 0;
                    if (buffer[strlen(buffer)-1]=='\n') buffer[strlen(buffer)-1] = '\0';
                    // Make a copy of the input line to tokenize
                    char* lineCopy = strdup(buffer);
                    if (lineCopy == NULL) {
                        perror("strdup failed");
                    }
                    // Tokenize the string and store words
                    char* word = strtok(lineCopy, " ");
                    while (word != NULL && wordCount < MAX_WORDS) {
                        words[wordCount] = strdup(word); // Duplicate the word
                        if (words[wordCount] == NULL) {
                            perror("strdup failed");
                            free(lineCopy);
                        }
                        wordCount++;
                        word = strtok(NULL, " ");
                    }
                    words[wordCount] = NULL;  // Null-terminate the array of strings
                    int pid;
                    free(lineCopy);
                    if (pid=fork() == 0) {
                        close(0);
                        dup(in_b);
                        int check = execvp(words[0],words);
                        if (check == -1) {
                            printf("*** Unable to execute command\n");
                        }
                        exit(0);
                    }
                    else waitpid(pid, NULL, WUNTRACED);
                    fflush(stdout);
                    // printf("\n");
                }
                if (swap == 1) {
                    char buffer[1024];
                    write(2,"Enter command> ", 15);
                    fgets(buffer,1024, stdin);
                    printf("%s",buffer);
                    fflush(stdout);
                    if (strcmp(buffer,"exit\n") == 0) {
                        exit(0);
                    }
                    if (strcmp(buffer,"swaprole\n") == 0) {
                        swap = 0;
                        close(0);
                        dup(a2);
                        close(1);
                        dup(out_b);
                        for (int i = 0; i < 1024; i++) buffer[i] = '\0';
                        continue;
                    }
                    fflush(stdout);
                }            
            }
        }
    }  
    exit(0);
}
