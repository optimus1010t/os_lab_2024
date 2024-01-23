#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define INFNAME "dup.c"
#define OUTFNAME "DUP.c"

int main ()
{
    int ifd, ofd;
    char c;

    /* Open input file descriptor */
    ifd = open(INFNAME, O_RDONLY);
    if (ifd < 0) {
        fprintf(stderr, "Unable to open input file in read mode...\n");
        exit(1);
    } else {
        fprintf(stderr, "New file descriptor obtained = %d\n", ifd);
    }

    int new_ifd = dup(ifd); int i = 0;
    while (1) {
        char ch[2] = {'\0', '\0'};
        if (i%2 == 0 ) read(new_ifd, ch, 1);
        else read(ifd, ch, 1);
        if (ch[0] == '\0') break;
        printf("%s", ch);
        i++;
    }
    exit(0);
}