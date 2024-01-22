/*
		EXECUTING AN APPLICATION USING EXEC COMMANDS
	The execvp() family of commands can be used to execute an
	application from a process. The system call execvp()
	replaces the executing process by a new process image
	which executes the application specified as its parameter.
	Arguments can also be specified. Refer to online man pages.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>

main()
{
	char *arglist[3];

    /*  "cal" is an application which shows the calendar of the
        current year and month. "cal" with an argument specifying
        year (for example "cal 1999") shows the calendar of that
        year. Try out the "cal" command from your command prompt.

        Here we execute "cal 2012" using the execvp() system call.
        Note that we specify "cal" in the first two arguments. The
        reason for this is given in the online man pages for execvp()
    */

	/* Initialize the argument list */
	arglist[0] = (char *)malloc(4*sizeof(char)); strcpy(arglist[0],"cal");
	arglist[1] = (char *)malloc(5*sizeof(char)); strcpy(arglist[1],"2012");
	arglist[2] = NULL;

	/* Call execvp */
	execvp("cal",arglist); 

    /*  The execvp() system call does not return. Note that the
        following statement will not be executed.
    */

    /*  The exec utility shall open, close, and/or copy file descriptors as specified by any redirections as part of the command.
        If exec is specified without command or arguments, and any file descriptors with numbers greater than 2 are opened with associated 
        redirection statements, it is unspecified whether those file descriptors remain open when the shell  invokes
        another utility.  Scripts concerned that child shells could misuse open file descriptors can always close them explicitly, as shown 
        in one of the following examples.
        If exec is specified with command, it shall replace the shell with command without creating a new process. If arguments are specified, 
        they shall be arguments to command.  Redirection affects the current shell execution environment. */

    /*  In simple words, when you open a file, the operating system creates an entry to represent that file and store the information about 
        that opened file. So if there are 100 files opened in your OS then there will be 100 entries in OS (somewhere in kernel). These entries 
        are represented by integers like (...100, 101, 102....). This entry number is the file descriptor. So it is just an integer number that 
        uniquely represents an opened file for the process. If your process opens 10 files then your Process table will have 10 entries for file 
        descriptors.
        Similarly, when you open a network socket, it is also represented by an integer and it is called Socket Descriptor. */

	printf("This statement is not executed if execvp succeeds.\n");
}