/*
    Name: Steven Culwell
    ID:   1001783662
*/

#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// TODO: Create documentation comments for each function
// TODO: Remove debug printing

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11    // Mav shell supports up to 10 arguments, in
                                // addition to the command

// Array structure containing current length,
// max length, and the data within the array
typedef struct array array;
struct array {
    int len;
    int max_len;
    char **data;
};

// Create new array list with array of data
// of size max_len
array *new_array(int max_len) {
    array *l = (array *) calloc(1, sizeof(array));
    l->data = (char **) calloc(max_len, sizeof(char *));
    l->max_len = max_len;
    l->len = 0;
    
    return l;
}

// Push all elements over one and insert new element
// at beginning of array
void array_push_front(array *l, char *data) {
    // Get rid of last element (if it exists)
    // so that we can overwrite it with the 
    // previous element
    if(l->data[l->max_len-1]) {
        free(l->data[l->max_len-1]);
        l->data[l->max_len] = NULL;
    }
    // Shift all elements 1 place forward
    for(int i = l->max_len-1; i >= 1; i--)
        l->data[i] = l->data[i-1];
    
    // First position should now be able to
    // be overwritten with new element
    l->data[0] = strndup(data, MAX_COMMAND_SIZE);
    // Increase current length, assuming we
    // aren't already at the end of the array
    if(l->len < l->max_len)
        l->len++;
}

// Get the element at position n in the array,
// starting from the current length and
// counting backwards.
char *array_get(array *l, int n) {
    if(n >= l->max_len)
        return NULL;
    return l->data[l->len - 1 - n];
}

// List all elements that have been set in
// the array, with their index being from
// the array length to 0
void array_list(array *l) {
    for(int i = l->len-1; i >= 0; i--)
        printf("%d: %s\n", l->len-1-i, l->data[i]);
}

// Free all allocated memebers of the array
// struct (each element in data, data, and array)
void array_free(array *l) {
    for(int i = 0; i < l->max_len; i++)
        free(l->data[i]);
    free(l->data);
    free(l);
}

// Returns true if we have permission to execute the file located
// at file_path, false otherwise
bool executable_exists(char *file_path) {
    return access(file_path, R_OK) == 0;
}

// Return true of str only contains digit characters, false otherwise
bool is_number(char *str) {
    int len = strnlen(str, MAX_COMMAND_SIZE);
    for(int i = 0; i < len; i++)
        if(!isdigit(str[i]))
            return false;
    return true;
}

// Search through path "location" for executable "command".
// If found return true and set command_location equal to
// the absolute path of the command executable. Otherwise
// return false
bool find_command_in_path(char *location, char *command, char *command_location) {
    char *new_path = strndup(location, MAX_COMMAND_SIZE+1);
    strncat(new_path, command, MAX_COMMAND_SIZE);
    if(executable_exists(new_path)) {
        strncpy(command_location, new_path, MAX_COMMAND_SIZE);
        free(new_path);
        return true;
    }
    free(new_path);
    return false;
}

// Check for the given command in current direcotry, "/usr/local/bin/",
// "/usr/bin", "/bin/" in that order. The found path is copied over to
// command_location so it can be run with exec.
// Will return true if successfully found a path, false otherwise
bool find_command(char *command, char *command_location) {
    return find_command_in_path("", command, command_location) ||
           find_command_in_path("/usr/local/bin/", command, command_location) ||
           find_command_in_path("/usr/bin/", command, command_location) ||
           find_command_in_path("/bin/", command, command_location);
}

// Spawn a new process by forking the current one,
// exec a new executable with args, and then wait
// on the newly spawned process to finish
pid_t spawn_process(char *command, char **args) {
    pid_t child = fork();
    if (child == 0) {
        execvp(command, args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } else {
        int status = 0;
        waitpid(child, &status, 0);
        return child;
    }
}

int main() {
    char *command_string = (char *) malloc(MAX_COMMAND_SIZE);
    char *command_location = (char *) malloc(MAX_COMMAND_SIZE);
    char *full_command = (char *) malloc(MAX_COMMAND_SIZE);
    char *pid_buffer = (char *) malloc(MAX_COMMAND_SIZE);
    array *pids = new_array(20);
    array *history = new_array(15);
    
    while(true) {
        // Print out the msh prompt
        printf("msh> ");
        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
        
        strncpy(full_command, command_string, MAX_COMMAND_SIZE);
        // Remove newline from command so that it is formatted correctly in history
        full_command[strnlen(full_command, MAX_COMMAND_SIZE)-1] = 0; 
        
        // Check if user entered "!n", where n is the index of a command in history.
        // If they did, we will replace the current command with the nth command in
        // history so that it can be tokenized
        if(strnlen(full_command, MAX_COMMAND_SIZE) > 1 &&
                full_command[0] == '!' &&
                is_number(&full_command[1])) {
                
            int idx = atoi(&full_command[1]);
            char *new_command_string = array_get(history, idx);
            if(new_command_string == NULL) {
                fprintf(stderr, "Command not in history.\n");
                continue;
            }
            strncpy(command_string, new_command_string, MAX_COMMAND_SIZE);
            strncpy(full_command, new_command_string, MAX_COMMAND_SIZE);
        }
        
        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *argument_ptr;

        char *working_string = strdup( command_string );
        
        // we are going to move the working_string pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *head_ptr = working_string;

        // Tokenize the input strings with whitespace used as the delimiter
        while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) &&
                (token_count<MAX_NUM_ARGUMENTS)) {
            token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
            if( strlen( token[token_count] ) == 0 )
            {
                free(token[token_count]);
                token[token_count] = NULL;
            }
            token_count++;
        }

        free( head_ptr );
        
        // If nothing has been entered, we will skip checking the input to avoid
        // a segmentation fault
        if(token_count == 0 || token[0] == NULL) {
            continue;
        }
        
        // Add current command to history
        array_push_front(history, full_command);
        
        // The shell will exit with status 0 if the command "quit" or "exit" is given
        if(strncmp(token[0], "exit", MAX_COMMAND_SIZE) == 0 ||
                strncmp(token[0], "quit", MAX_COMMAND_SIZE) == 0) {
            exit(0);
        }
        // check for built-in "cd", change directory to the first argument given
        else if(strncmp(token[0], "cd", MAX_COMMAND_SIZE) == 0) {
            // If more than 3 tokens (command, 1st argument, NULL) are present,
            // then too many arguments have been passed to cd.
            // cd will only accept 1 argument, which is the
            // directory to switch to
            if(token_count > 3) {
                fprintf(stderr, "cd: Too many arguments\n");
            }
            // If we cannot change directory, show error
            else if(chdir(token[1]) == -1) {
                perror("cd");
            }
        }
        // List pids of last 20 spawned processes
        else if(strncmp(token[0], "listpids", MAX_COMMAND_SIZE) == 0) {
            array_list(pids);
        }
        // List last 15 commands
        else if(strncmp(token[0], "history", MAX_COMMAND_SIZE) == 0) {
            array_list(history);
        }
        // Command is not built in command, look for executable
        // and run it with given arguments. Add command to history
        else if(find_command(token[0], command_location)) {
            pid_t child = spawn_process(command_location, token);
            
            snprintf(pid_buffer, MAX_COMMAND_SIZE, "%d", child);
            array_push_front(pids, pid_buffer);
        }
        // Otherwise, the given command cannot be recognized.
        // Show corresponding error message
        else {
            printf("%s: Command not found.\n", token[0]);
        }
        
        // Free all non-null tokens
        for(int i = 0; i < token_count; i++) {
            if(token[i])
                free(token[i]);
        }
    }
    
    array_free(pids);
    array_free(history);
    free(command_string);
    free(command_location);
    free(full_command);
    free(pid_buffer);
    
    return EXIT_SUCCESS;
}
