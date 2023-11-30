#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extract the filename from the command line argument
    char *filename = argv[1];

    // Check if the file exists
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "The file %s does not exist.\n", filename);
        exit(EXIT_FAILURE);
    }

    // Build the command: taskset -c 8 filename
    char command[256];
    snprintf(command, sizeof(command), "taskset -c 0-7 %s", filename);

    // Execute the command using system()
    int status = system(command);

    // Check if the command execution was successful
    if (status == -1) {
        perror("Error executing command");
        exit(EXIT_FAILURE);
    } else if (WIFEXITED(status)) {
        // Check the exit status of the command
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            fprintf(stderr, "Command exited with status %d\n", exit_status);
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
