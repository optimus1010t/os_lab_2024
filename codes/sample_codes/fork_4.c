#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main ()
{
    int A[10], *B, i, t, pid;
    srand((unsigned int)time(NULL));
    /*You can pass in a pointer to a time_t object that time will fill up with the current time (and the return value is the same 
    one that you pointed to). If you pass in NULL, it just ignores it and merely returns a new time_t object that represents the 
    current time.
    The call to time(NULL) returns the current calendar time (seconds since Jan 1, 1970). Ordinarily, if you pass in a pointer to 
    a time_t variable, that pointer variable will point to the current time.*/

    B = (int *)malloc(10 * sizeof(int));
    t = 1 + rand() % 5;
    if ((pid = fork())) {
        for (i=0; i<10; ++i) A[i] = B[9-i] = 10+i;
        printf("Parent process going to sleep for %d seconds\n", t);
        sleep(t);
        printf("Parent process: A = %p, B = %p\n", A, B);  /* %p is for printing a pointer address. 85 in decimal is 55 in hexadecimal. 
        On your system pointers are 64bit (16 bit in hexadecimal), so the full hexidecimal representation is: 0000000000000055. */
        } 
    else {
        for (i=0; i<10; ++i) A[i] = B[9-i] = i;
        i = t; while (i == t) t = 1 + rand() % 5;
        printf("Child process going to sleep for %d seconds\n", t);
        sleep(t);
        printf("Child process: A = %p, B = %p\n", A, B);   /* same address as before 
        Child process: A = 0x7ffd42b07030, B = 0x5624b22802a0
        A[] = 0 1 2 3 4 5 6 7 8 9
        B[] = 9 8 7 6 5 4 3 2 1 0
        Parent process: A = 0x7ffd42b07030, B = 0x5624b22802a0 */
    }
    printf("A[] ="); for (i=0; i<10; ++i) printf(" %d", A[i]); printf("\n");
    printf("B[] ="); for (i=0; i<10; ++i) printf(" %d", B[i]); printf("\n");
    free(B);
    if (pid) wait(NULL);
    /*  You actually don't call wait() , wait takes one int* argument that points to a integer where the child's exit status will 
        be stored. If you don't care about the child's exit status (which is quite often), you can just call wait(NULL) . wait waits 
        for the termination of any child process.*/
    exit(0);
}