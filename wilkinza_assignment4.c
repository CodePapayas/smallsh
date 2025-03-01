#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>


// Global Variables
char *user_input = NULL; // User input
size_t buffer = 0; // Buffer size
char *token_arr[512]; // Arguments
bool background_mode = false; // Background process flag
int i = 0; // Counter
int last_exit_status = 0; // Exit status of last foreground process

// Get user input
// Adapted from https://opensource.com/article/22/5/safely-read-user-input-getline
void tokenize_input(char *tokens[]) {
    char *token = strtok_r(user_input, " \n", &user_input);
    int i = 0;
    while (token != NULL) {
        token_arr[i] = token;
        i++;
        token = strtok_r(NULL, " \n", &user_input);
    }
    token_arr[i] = NULL;
    };

// Flag check for background process
void check_background(char *tokens[]) {
    int i = 0;
    // Determine length of token array
    while (tokens[i] != NULL) {
        i++;
    }
    // Check for ampersand at end of input
    if (strcmp(tokens[i-1], "&") == 0) {
        background_mode = true;
        tokens[i-1] = NULL;  // Remove ampersand from token array
    } else {
        background_mode = false;  // Default to false
    }
}

// Check for empty or comment lines, continue to prompt user for input
bool empty_or_comment(char *tokens[]) {
    if (tokens[0] == NULL || tokens[0][0] == '#') {
        return false;
    }
    else if (strspn(tokens[0], " \t") == strlen(tokens[0])) {
        return false;
    }
    return true;
}

bool check_built_in(char *tokens[]) {
    if (strcmp(tokens[0], "exit") == 0) {
        exit(0);
    }
    else if (strcmp(tokens[0], "cd") == 0) {
        const char *dir = (tokens[1] != NULL) ? tokens[1] : getenv("HOME");
        if (chdir(dir) != 0) {
            perror("cd");
        }
        return true;
    }
    else if (strcmp(tokens[0], "status") == 0) {
        printf("Exit status: %d\n", last_exit_status);
        return true;
    }
    return false;
}

void get_input() {
    printf(": ");
    fflush(stdout);
    size_t input_ln = getline(&user_input, &buffer, stdin);

    if (input_ln > 2048 || input_ln < 0) {
        printf("Error: Invalid input\n");
        fflush(stdout);
        exit(1);
    }

    tokenize_input(token_arr);
}

void print_input(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("%s ", tokens[i]);
        fflush(stdout);
        i++;
    }
    printf("\n");
    fflush(stdout);
}


int main() {
    do {
        get_input();
    } while (!empty_or_comment(token_arr));
    if (!check_built_in(token_arr)) {
    check_background(token_arr);
    if (background_mode) {
        printf("Background mode enabled\n");
        fflush(stdout);
    }
    print_input(token_arr);
    fflush(stdout);
}
    return 0;
}