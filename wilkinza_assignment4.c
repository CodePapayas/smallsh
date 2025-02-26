#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h> 


// Global Variables
char *user_input = NULL; // User input
size_t buffer = 0; // Buffer size
char *token_arr[512]; // Arguments

// Adapted from https://opensource.com/article/22/5/safely-read-user-input-getline
void get_input() {
    // Get user input
    printf(": ");
    fflush(stdout);
    getline(&user_input, &buffer, stdin);
    puts(user_input);
    fflush(stdout);
}

int main() {
    // Get user input
    get_input();
    return 0;
}