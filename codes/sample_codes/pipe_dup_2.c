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

    /* Open output file descriptor */
    /* The file is created in the mode rw-r--r-- (644) */
    ofd = open(OUTFNAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ofd < 0) {
        fprintf(stderr, "Unable to open output file in write mode...\n");
        exit(2);
    } else {
        fprintf(stderr, "New file descriptor obtained = %d\n", ofd);
    }

    close(0);    /* Close the file descriptor for stdin */
    close(1);    /* Close the file descriptor for stdout */

    dup(ifd);    /* Duplicate ifd at the lowest-numbered unused descriptor */
    close(ifd);  /* ifd is no longer needed */

    dup(ofd);    /* Duplicate ofd at the lowest-numbered unused descriptor */
    close(ofd);  /* ofd is no longer needed */

    /* Read from stdin and write to stdout, as if nothing has happenned */
    while (1) {
        scanf("%c", &c);  /* Reading is done from the input file */
        if (feof(stdin)) break; /*  feof function: The feof function is a standard library function in C that checks if 
        the end-of-file indicator for a given file stream is set. This function is often used with file streams to 
        determine if the end of a file has been reached during reading. However, in this context, feof is used with stdin. 
        
        Parameter is the stream that is FILE* stream  */

        /* Under both Windows and Linux, dup() will duplicate the file descriptor, but both descriptors still point to 
        the same file structure in the process' file table. Any seeking on either descriptor will adjust the position 
        for the other descriptors as well. */
        if ((c >= 'a') && (c <= 'z')) c -= 'a' - 'A';
        printf("%c", c);  /* Writing is done to the output file */
    }
    exit(0);
}