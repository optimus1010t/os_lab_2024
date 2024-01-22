#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>

#define MAX_WORD_LEN 1000                        // Defining the maximum length of the buffer to store number of children as string

char * extractword( const char str[], size_t pos )  // Creating a function to extract the word from the string at the desired position
{
    const char delimiter[] = " \t";
    char *word_input = malloc( ( strlen( str ) + 1 ) );
    char *our_word = NULL;
    if ( word_input != NULL )
    {
        strcpy( word_input, str );
        our_word = strtok( word_input, delimiter );
        while ( our_word != NULL && pos -- != 0 )
            our_word = strtok( NULL, delimiter );
        if ( our_word == NULL )
            free( word_input );
        if ( our_word != NULL ) {
            size_t n = strlen( our_word );
            memmove( word_input, our_word, n + 1 );
            our_word = realloc( word_input, n + 1 );
        }
    }
    return our_word;
}

int main(int argc, char *argv[]) {
    int spaces;
    char *str;
    if (argc == 1) {                // If no arguments are given, print the usage and exit
        printf("Run with a node name\n");
        exit(1);
    }
    if (argc == 2) {                // If only one argument is given, set spaces to 0
        spaces = 0;
    } else {
        spaces = atoi(argv[2]);
    }
    if (spaces < 0) {               // If spaces is negative, print the usage and exit
        printf("Usage: ./proctree <string> <level> (where level is a non-negative integer)\n");
        exit(1);
    }
    str = argv[1];
    FILE *fp = fopen("treeinfo.txt", "r");  // Open the file in read mode and the textfile is assumed to be in the current working directory
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int str_len = strlen(str);
    int check = 1;

    while((read = getline(&line, &len, fp)) != -1) {    // Read the file line by line
        if (strncmp(line, str, str_len) == 0) {         // If the city is found, break
            check = 0;
            break;
        }
    }

    if (check == 1) {                                   // If the city is not found, print error and exit
        printf("City %s not found\n", str);
        exit(1);
    }

    int my_pid = getpid();                              // Get the pid of the current process
    for (int i = 0; i < spaces; i++) {                  // Print the spaces
        printf("\t");
    }
    printf("%s (%d)\n", str, my_pid);                   // Print the city and the pid

    line[strcspn(line, "\n")] = 0;                      // Remove the newline character from the line
    fclose(fp);
    char *token = extractword(line, 1);                 // Extract the number of children from the line
    int num = atoi(token);

    for (int i = 0; i < num; i++) {
        token = extractword(line, i + 2);               // Extract the children from the line
        pid_t pid = fork();
        if (pid == 0) {
            char spacey[MAX_WORD_LEN];
            sprintf(spacey, "%d", spaces + 1);          // Convert the number of spaces to a string
            char *args[] = {"./proctree", token, spacey, NULL}; // Create the arguments for the execvp command that are the command and the arguments
            execvp(args[0], args);                      // Execute the command
        } else {
            wait(NULL);                                 // Wait for the child process to finish
        }
    }    
    exit(0);
}
