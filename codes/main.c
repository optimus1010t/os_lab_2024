#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <errno.h>

void f(int n)
{
    if (n  < 2){
        printf("hello world\n");
        exit(0);
    }
    if (fork()!=0) f(n-1);
    if (fork()!=0) f(n-2);
}

int main(void)
{
    f(3);
    return 0;
}