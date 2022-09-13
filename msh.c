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

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

typedef struct _queue queue;
struct _queue {
    size_t size;
    char **data;
};

// Create new queue
queue *new_queue(size_t size) {
    queue *q = (queue *) malloc(sizeof(queue));
    q->data = (char **) calloc(size, sizeof(char *));
    q->size = size;
    return q;
}

// List each entry in queue labeled 1..n
// Will stop once a NULL entry is found
// because that is one past the last
// entry in the queue
void queue_list(queue *q) {
    for(size_t i = 0; i < q->size; i++) {
        if(q->data[i] == NULL)
            break;
        printf("%ld: %s\n", i+1, q->data[i]);
    }
}

// TODO: The queue is backwards, 0th command should be oldest, 15th command should be newest
//       Reverse order of queue
void enqueue(queue *q, char *data) {
    // Free last item on queue if it exists so
    // that it can be overwritten in following
    // for loop
    if(q->data[q->size-1] != NULL)
        free(q->data[q->size-1]);
    // Shift all items in queue down to make space
    // for new item at front of queue
    for(size_t i = q->size-1; i >= 1; i--) {
        q->data[i] = q->data[i-1];
    }
    q->data[0] = strndup(data, MAX_COMMAND_SIZE);
}

char *queue_get(queue *q, int n) {
    if(n >= q->size)
        return NULL;
    return q->data[n];
}

// Free all items in queue, as well as the queue itself
void queue_free(queue *q) {
    for(size_t i = 0; i < q->size; i++) {
        if(q->data[i])
            free(q->data[i]);
    }
    free(q);
}

bool file_exists(char *file_name) {
    return access(file_name, F_OK) == 0;
}

bool is_number(char *str) {
    int len = strnlen(str, MAX_COMMAND_SIZE);
    for(int i = 0; i < len; i++) {
        if(!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

bool find_command_in_path(char *location, char *command, char *command_location) {
    char *new_path = strndup(location, MAX_COMMAND_SIZE);
    strncat(new_path, command, MAX_COMMAND_SIZE);
    // printf(" # Looking for %s\n", new_path);
    if(file_exists(new_path)) {
        // printf(" # Found it!\n");
        strncpy(command_location, new_path, MAX_COMMAND_SIZE);
        return true;
    }
    return false;
}

bool find_command(char *command, char *command_location) {
    // Check if the command exists within the current directory first
    // If it does, we will copy command over into command_location so
    // that the command in the current directory is executed
    // printf(" # Looking for ./%s\n", command);
    if(file_exists(command)) {
        // printf(" # Found it!\n");
        strncpy(command_location, command, MAX_COMMAND_SIZE);
        return true;
    }
    
    // Check for the given command in "/usr/local/bin/", "/usr/bin", "/bin/"
    // in that order. The found path is copied over to command_location
    // so it can be run with exec. Will return true if successfully found
    // a path, false otherwise
    return find_command_in_path("/usr/local/bin/", command, command_location) ||
           find_command_in_path("/usr/bin/", command, command_location) ||
           find_command_in_path("/bin/", command, command_location);
}

int main() {
    char *command_string = (char *) malloc(MAX_COMMAND_SIZE);
    char *command_location = (char *) malloc(MAX_COMMAND_SIZE);
    queue *pids = new_queue(20);
    queue *history = new_queue(15);
    
    while(true) {
        // Print out the msh prompt
        printf("msh> ");
        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
        
        // Remove newline from command so that it is formatted correctly in history
        command_string[strnlen(command_string, MAX_COMMAND_SIZE)-1] = 0; 

        if(strnlen(command_string, MAX_COMMAND_SIZE) > 1 &&
                command_string[0] == '!' &&
                is_number(&command_string[1])) {
                
            int idx = atoi(&command_string[1]);
            char *new_command_string = queue_get(history, idx);
            if(new_command_string == NULL) {
                fprintf(stderr, "Command not in history.\n");
                continue;
            }
            printf("%s\n", new_command_string);
            command_string = strndup(new_command_string, MAX_COMMAND_SIZE);
        }
        
        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int token_count = 0;                                 
                                                               
        // Pointer to point to the token
        // parsed by strsep
        char *argument_ptr;                                         
                                                               
        char *working_string = strdup( command_string );                
        
        char *full_command = strndup(command_string, MAX_COMMAND_SIZE);

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
                token[token_count] = NULL;
            }
            token_count++;
        }

        free( head_ptr );
        
        // If nothing has been entered, we will skip checking the input to avoid
        // a segmentation fault
        if(token_count != 0 && token[0] != NULL) {
            enqueue(history, full_command);
            free(full_command);
        } else {
            free(full_command);
            continue;
        }
        
        
        if(strncmp(token[0], "exit", MAX_COMMAND_SIZE) == 0 ||
                strncmp(token[0], "quit", MAX_COMMAND_SIZE) == 0) {
            exit(EXIT_SUCCESS);
        } else if(strncmp(token[0], "cd", MAX_COMMAND_SIZE) == 0) {
            // If more than 3 tokens (command, 1st argument, NULL) are present, 
            // then too many arguments have been passed to cd.
            // cd will only accept 1 argument, which is the
            // directory to switch to
            if(token_count > 3) {
                fprintf(stderr, "cd: Too many arguments\n");
            } else {
                int result = chdir(token[1]);
                if(result == -1) {
                    perror("cd");
                }
            }
        } else if(strncmp(token[0], "listpids", MAX_COMMAND_SIZE) == 0) {
            queue_list(pids);
        } else if(strncmp(token[0], "history", MAX_COMMAND_SIZE) == 0) {
            queue_list(history);
        } else if(find_command(token[0], command_location)) {
            pid_t child = fork();
            if (child == 0) {
                // printf(" # execvp %s\n", command_location);
                execvp(command_location, &token[0]);
            } else {
                int status = 0;
                char buffer[MAX_COMMAND_SIZE] = { 0 };
                sprintf(buffer, "%d", child);
                enqueue(pids, buffer);
                waitpid(child, &status, 0);
            }
        }  else {
            printf("%s: Command not found.\n", token[0]);
        }

    }
    queue_free(pids);
    queue_free(history);
    free(command_string);
    free(command_location);
    return EXIT_SUCCESS;
}
