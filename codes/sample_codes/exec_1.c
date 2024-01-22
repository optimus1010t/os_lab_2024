/*
		EXECUTING AN APPLICATION USING EXEC COMMANDS
	The execlp() family of commands can be used to execute an
	application from a process. The system call execlp()
	replaces the executing process by a new process image
	which executes the application specified as its parameter.
	Arguments can also be specified. Refer to online man pages.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>

main()
{
    /*  "cal" is an application which shows the calendar of the
        current year and month. "cal" with an argument specifying
        year (for example "cal 1999") shows the calendar of that
        year. Try out the "cal" command from your command prompt.

        Here we execute "cal 2012" using the execlp() system call.
        Note that we specify "cal" in the first two arguments. The
        reason for this is given in the online man pages for execlp()
	*/

	execlp("cal\0","cally","2012\0",(char *)0);              // can or cannot be null  terminated strings

    /*  The execlp() system call does not return. Note that the
        following statement will not be executed.
	*/

    // The following code forks and changes directory of that shell not the current shell from where we executed ./a.out
    // char s[100];
    // printf("%s\n", getcwd(s, 100)); 
    // chdir(".."); 
    // printf("%s\n", getcwd(s, 100));

	printf("This statement is not executed if execlp succeeds.\n");
    exit(0);
}