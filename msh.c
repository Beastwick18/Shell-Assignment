/*
    Name: Steven Culwell
    ID:   1001783662
*/

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

typedef struct llist_node llist_node;
struct llist_node {
    struct llist_node *next;
    struct llist_node *prev;
    char *data;
};

typedef struct llist llist;
struct llist {
    struct llist_node *head;
    struct llist_node *tail;
    int visible_length;
};

llist *new_llist(int visible_length) {
    llist *ll = (llist *) calloc(1, sizeof(llist));
    ll->visible_length = visible_length;
    return ll;
}

llist_node *new_llist_node(llist_node *prev, char *data) {
    llist_node *node = (llist_node *) calloc(1, sizeof(llist_node));
    node->prev = prev;
    node->data = strndup(data, MAX_COMMAND_SIZE);
    return node;
}

void llist_push_back(llist *ll, char *data) {
    if(ll == NULL) {
        return;
    }
    if(ll->head == NULL) {
        ll->head = new_llist_node(NULL, data);
        ll->tail = ll->head;
        return;
    }
    ll->tail->next = new_llist_node(ll->tail, data);
    ll->tail = ll->tail->next;
}

llist_node *llist_go_back(llist_node *tail, int back) {
    if(back == 0) {
        return tail;
    }
    if(tail->prev == NULL) {
        return tail;
    }
    return llist_go_back(tail->prev, back-1);
}

void llist_list_recursive(llist_node *node, int index) {
    if(node == NULL)
        return;
    printf("%d: %s\n", index, node->data);
    llist_list_recursive(node->next, index+1);
}

void llist_list(llist *ll) {
    llist_node *starting_node = llist_go_back(ll->tail, ll->visible_length-1);
    llist_list_recursive(starting_node, 1);
}

char *llist_get_recursive(llist_node *node, int index) {
    if(node == NULL)
        return NULL;
    if(index == 0)
        return node->data;
    return llist_get_recursive(node->next, index-1);
}

char *llist_get(llist *ll, int index) {
    if(index < 1 || index > ll->visible_length)
        return NULL;
    llist_node *starting_node = llist_go_back(ll->tail, ll->visible_length-1);
    return llist_get_recursive(starting_node, index-1);
}

void llist_free_recursive(llist_node *head) {
    if(head == NULL)
        return;
    llist_free_recursive(head->next);
    free(head->data);
    free(head);
}

void llist_free(llist *ll) {
    llist_free_recursive(ll->head);
    free(ll);
}

bool file_exists(char *file_name) {
    return access(file_name, F_OK) == 0;
}

bool is_number(char *str) {
    int len = strnlen(str, MAX_COMMAND_SIZE);
    for(int i = 0; i < len; i++)
        if(!isdigit(str[i]))
            return false;
    return true;
}

bool find_command_in_path(char *location, char *command, char *command_location) {
    char *new_path = strndup(location, MAX_COMMAND_SIZE+1);
    strncat(new_path, command, MAX_COMMAND_SIZE);
    if(file_exists(new_path)) {
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
    llist *pids = new_llist(20);
    llist *history = new_llist(15);
    
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
            char *new_command_string = llist_get(history, idx);
            if(new_command_string == NULL) {
                fprintf(stderr, "Command not in history.\n");
                continue;
            }
            // printf("%s\n", new_command_string);
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
        llist_push_back(history, full_command);
        
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
            } else {
                int result = chdir(token[1]);
                if(result == -1) {
                    perror("cd");
                }
            }
        } else if(strncmp(token[0], "listpids", MAX_COMMAND_SIZE) == 0) {
            llist_list(pids);
        } else if(strncmp(token[0], "history", MAX_COMMAND_SIZE) == 0) {
            llist_list(history);
        } else if(find_command(token[0], command_location)) {
            pid_t child = spawn_process(command_location, &token[0]);
            
            char buffer[MAX_COMMAND_SIZE] = { 0 };
            snprintf(buffer, MAX_COMMAND_SIZE, "%d", child);
            llist_push_back(pids, buffer);
        }  else {
            printf("%s: Command not found.\n", token[0]);
        }
        
        for(int i = 0; i < token_count; i++) {
            free(token[i]);
        }
    }
    llist_free(pids);
    llist_free(history);
    free(command_string);
    free(command_location);
    free(full_command);
    return EXIT_SUCCESS;
}
