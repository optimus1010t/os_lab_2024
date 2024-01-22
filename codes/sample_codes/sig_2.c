#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void sigint_handler(int sig)
{
        /* using a char[] so that sizeof will work */
    const char msg[] = "Ahhh! SIGINT!\n";
    write(1, msg, sizeof(msg));
}
int main(void)
{
    char s[200];
/*  sa_handler  The signal handler function (or SIG_IGN to ignore the signal)
    sa_mask	    A set of signals to block while this one is being handled
    sa_flags	Flags to modify the behavior of the handler, or 0 */

    struct sigaction sa = {
        .sa_handler = sigint_handler,
        .sa_flags = SA_RESTART,
        .sa_mask = 0,
    };

/*  The first parameter, sig is which signal to catch. This can be (probably “should” be) a symbolic name 
    from signal.h along the lines of SIGINT. That’s the easy bit.

    The next field, act is a pointer to a struct sigaction which has a bunch of fields that you can fill in to 
    control the behavior of the signal handler. (A pointer to the signal handler function itself included in the struct.)

    Lastly oact can be NULL, but if not, it returns the old signal handler information that was in place before. This is 
    useful if you want to restore the previous signal handler at a later time.*/

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Enter a string:\n");

    // restart:{                                                        // method 1
    //     if (fgets(s, sizeof s, stdin) == NULL) goto restart;
    //     printf("You entered: %s\n", s);
    //     exit(0);
    // }
    if (fgets(s, sizeof s, stdin) == NULL)                              // method 2
        perror("fgets");
    else 
        printf("You entered: %s\n", s);

    return 0;
}