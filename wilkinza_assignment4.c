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
char *token_arr[512]; // Arguments
int bg_pid_count = 0; // Number of background processes
int i = 0; // Counter
int last_exit_status = 0; // Exit status of last foreground process
pid_t bg_pids_arr[512]; // Array of background process IDs
size_t buffer = 0; // Buffer size
bool background_mode = false; // Background process flag
bool foreground_mode = false; // Foreground process flag

// Handle signals
void handle_SIGTSTP(int signal) {
    char *message;
    if (foreground_mode) {
        message = "\nExiting foreground-only mode\n: ";
        foreground_mode = false;
    } else {
        message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        foreground_mode = true;
    }
    write(STDOUT_FILENO, message, strlen(message));
}

// Cleanup background processes
// Adapted from Exploration: Process API - Monitoring Child Processes CS374
void cleanup_background() {
    int status;
    for (int i = 0; i < bg_pid_count; i++) {
        if (waitpid(bg_pids_arr[i], &status, WNOHANG) > 0) {
            if (WIFEXITED(status)) {
                printf("Background process %d exited with status %d\n", bg_pids_arr[i], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Background process %d terminated by signal %d\n", bg_pids_arr[i], WTERMSIG(status));
            }
            for (int j = i; j < bg_pid_count; j++) {
                bg_pids_arr[j] = bg_pids_arr[j + 1];
            }
            bg_pid_count--;
        }
    }
}

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

// Check for redirection
// Adapted from Exploration: Processes and I/O CS374
void check_redirect(char *tokens[]) {
    int input_fd = -1, output_fd = -1;
    int i = 0;

    while (tokens[i] != NULL) {
        // Check for empty value; update exit status
        if (strcmp(tokens[i], "<") == 0) {
            if (tokens[i + 1] == NULL) {
                printf("Error: No input file specified\n");
                last_exit_status = 1;
                return;
            } // Open file for stdin
            input_fd = open(tokens[i + 1], O_RDONLY);
            if (input_fd == -1) {
                perror("source open()");
                last_exit_status = 1;
                return;
            }
            if (dup2(input_fd, 0) == -1) {
                perror("source dup2()");
                last_exit_status = 1;
                return;
            }
            close(input_fd);

            // Remove redirection tokens from token array
            for (int j = i; tokens[j] != NULL; j++) {
                tokens[j] = tokens[j + 2];
            }
            i--;
        }
        else if (strcmp(tokens[i], ">") == 0) {
            if (tokens[i + 1] == NULL) {
                printf("Error: No output file specified\n");
                last_exit_status = 1;
                return;
            }
            // Open file for stdout
            output_fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd == -1) {
                perror("target open()");
                last_exit_status = 1;
                return;
            }
            if (dup2(output_fd, 1) == -1) {
                perror("target dup2()");
                last_exit_status = 1;
                return;
            }
            close(output_fd);

            for (int j = i; tokens[j] != NULL; j++) {
                tokens[j] = tokens[j + 2];
            }
            i--;
        }
        i++;
    }
}

// Check for built-in commands
bool check_built_in(char *tokens[]) {
    if (strcmp(tokens[0], "exit") == 0) {
        cleanup_background();
        printf("Exiting shell\n");
        fflush(stdout);
        exit(0);
    }
    else if (strcmp(tokens[0], "cd") == 0) {
        const char *dir = (tokens[1] != NULL) ? tokens[1] : getenv("HOME");
        if (chdir(dir) != 0) {
            perror("cd");
        }
        return true;
    }
    // Check if terminated by signal or exit status
    else if (strcmp(tokens[0], "status") == 0) {
        if (last_exit_status >= 0) {
            printf("exit value %d\n", last_exit_status);
        } else {
            printf("terminated by signal %d\n", -last_exit_status);
        }
        fflush(stdout);
        return true;
    }    
    return false;
}

// Check for external commands
void check_external(char *tokens[]) {
    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {  // Child process
        struct sigaction default_action = {0};
        default_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_action, NULL);  // Allow SIGINT in foreground
        sigaction(SIGTSTP, &default_action, NULL); // Allow SIGTSTP

        check_redirect(tokens);
        if (execvp(tokens[0], tokens) == -1) {
            perror("execvp");
            exit(1);
        }
    } else {  // Parent process
        if (!background_mode || foreground_mode) {  // Foreground execution
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status);
            // Store signal as a negative for terminated processes
            // Allows us to tell the difference between exit status and signal
            } else if (WIFSIGNALED(status)) {
                last_exit_status = -WTERMSIG(status);
                printf("terminated by signal %d\n", -last_exit_status);
                fflush(stdout);
            }
        } else {  // Background execution
            bg_pids_arr[bg_pid_count] = pid;
            bg_pid_count++;
            printf("background pid is %d\n", pid);
            fflush(stdout);
        }
    }
}

// Get user input
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

// Strip ampersand from token array if at the end
void strip_ampersand(char *tokens[]) {
    int count = 0;
    while (tokens[count] != NULL) {
        count++;
    }
    if (count > 0 && strcmp(tokens[count - 1], "&") == 0) {
        tokens[count - 1] = NULL;
    }
}


int main() {
    struct sigaction SIGTSTP_action = {0}, SIGINT_action = {0};
    // Handle SIGINT in shell (ignore it)
    SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Handle SIGTSTP in shell
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while (1) {
        get_input();
    if (!empty_or_comment(token_arr)) {
        continue;
    }
    if (!check_built_in(token_arr)) {
        check_background(token_arr);
        check_external(token_arr);
    }
    cleanup_background();
    }
    return 0;
}
