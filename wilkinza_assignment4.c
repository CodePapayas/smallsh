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

// Get user input
// Adapted from https://opensource.com/article/22/5/safely-read-user-input-getline
void tokenize_input() {
    char *token = strtok_r(user_input, " \n", &user_input);
    int i = 0;
    while (token != NULL) {
        token_arr[i] = token;
        i++;
        token = strtok_r(NULL, " \n", &user_input);
    }
    token_arr[i] = NULL;

    for (int j = 0; j < i; j++) {
        printf("token_arr[%d]: %s\n", j, token_arr[j]);
        fflush(stdout);
    }
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

    tokenize_input();

    puts(user_input);
    fflush(stdout);
}


int main() {
    // Get user input
    get_input();
    return 0;
}