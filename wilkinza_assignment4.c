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
    print_input(token_arr);
    fflush(stdout);
    return 0;
}