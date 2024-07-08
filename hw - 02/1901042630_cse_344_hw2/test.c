#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

// Define paths for the FIFOs for easier reference
#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"

// Global counter to track how many child processes have terminated
volatile sig_atomic_t children_terminated = 0;

// Function to clean up resources, specifically unlinking FIFOs
void cleanup_resources() {
    unlink(FIFO1);
    unlink(FIFO2);
    printf("Resources cleaned up.\n");
}

// Handler for SIGINT (Ctrl+C), ensures clean exit by calling cleanup_resources
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("SIGINT received, cleaning up resources.\n");
        cleanup_resources();
        exit(EXIT_SUCCESS);
    }
}

// SIGCHLD handler to catch terminated children, print their status, and count them
void sigchld_handler(int sig) {
    int status; // Variable to store the exit status of the terminated child
    pid_t pid;  // Variable to store the PID of the terminated child

    // Loop to reap all terminated children without blocking
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Print the PID and exit status of the child process
        printf("Child with PID %d exited with status %d\n", pid, WEXITSTATUS(status));
        // Increment the count of terminated children
        children_terminated++;
    }
}

// Setup signal handlers for SIGCHLD and SIGINT
void setup_signal_handlers() {
    struct sigaction sa; // Struct to specify the action to be associated with SIGCHLD and SIGINT
    sigemptyset(&sa.sa_mask); // Initialize the signal set to empty, ensuring no signals are blocked during the execution of the handler
    sa.sa_flags = SA_RESTART; // Use SA_RESTART to make certain system calls restartable across signals

    // Set sigchld_handler as the handler function for SIGCHLD
    sa.sa_handler = sigchld_handler;
    // Apply the signal action to SIGCHLD
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Error setting SIGCHLD handler"); // If sigaction fails, print error message
        exit(EXIT_FAILURE); // Exit if there is an error setting the signal
    }

    // Set signal_handler as the handler function for SIGINT
    sa.sa_handler = signal_handler;
    // Apply the signal action to SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting SIGINT handler"); // If sigaction fails, print error message
        exit(EXIT_FAILURE); // Exit if there is an error setting the signal
    }

    // Register cleanup_resources to be called upon program exit
    atexit(cleanup_resources); // This ensures that cleanup_resources is called when exit() is called anywhere in the program
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size_of_array>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *endptr; // Pointer to the end of the parsed string
    long size = strtol(argv[1], &endptr, 10); // Convert string to long

    // Check for non-numeric input, no input, and integer-specific errors
    if (endptr == argv[1] || *endptr != '\0' || errno == ERANGE) {
        fprintf(stderr, "Invalid number: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    // Check for non-positive size of the array
    if (size <= 0) {
        fprintf(stderr, "Array size must be a positive integer, given: %ld\n", size);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    
    int numbers[size];
    for (int i = 0; i < size; i++) {
        numbers[i] = rand() % 10;  // Initialize array with values
    }

    printf("Array : [");
    for (int i = 0; i < size-1; i++) {
        printf("%d, ",numbers[i]);  // Printing array
    }
    printf("%d]\n\n",numbers[size-1]);


    setup_signal_handlers();

    // Create FIFOs for IPC
    if (mkfifo(FIFO1, 0666) == -1) {
        perror("Failed to create FIFO1");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(FIFO2, 0666) == -1) {
        perror("Failed to create FIFO2");
        exit(EXIT_FAILURE);
    }

    // Forking to create Child 1
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Failed to fork child 1");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) { // Child 1 process
        sleep(10); // Delay to simulate processing time
        int fifo1 = open(FIFO1, O_RDONLY);
        int sum = 0, temp;

        // Read integers from FIFO1 and calculate their sum
        while (read(fifo1, &temp, sizeof(temp)) > 0) {
            sum += temp;
        }
        printf("summation result: %d\n", sum);
        
        // Send sum to FIFO2 and a command to multiply
        int fifo2 = open(FIFO2, O_WRONLY);
        write(fifo2, &sum, sizeof(sum));
        write(fifo2, "multiply", sizeof("multiply"));
        close(fifo1);
        close(fifo2);
        exit(EXIT_SUCCESS);
    }

    // Forking to create Child 2
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Failed to fork child 2");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) { // Child 2 process
        sleep(10);
        int fifo2 = open(FIFO2, O_RDONLY);
        int sum;
        char command[10];

        // Read the sum and command from FIFO2
        read(fifo2, &sum, sizeof(sum));
        read(fifo2, command, sizeof(command));

        // Check command and perform multiplication if correct
        if (strcmp(command, "multiply") == 0) {
            int product = 1;
            int finalSum = 0;
            for (int i = 0; i < size; i++) {
                product *= numbers[i];
            }
            finalSum = sum + product;
            printf("multiplication result: %d\n", product);
            printf("FINAL RESULT => %d\n",finalSum);
        }

        close(fifo2);
        exit(EXIT_SUCCESS);
    }

    // Parent process writing to FIFO1
    int fifo1 = open(FIFO1, O_WRONLY);
    for (int i = 0; i < size; i++) {
        if (write(fifo1, &numbers[i], sizeof(numbers[i])) == -1) {
            perror("Failed to write to FIFO1");
            exit(EXIT_FAILURE);
        }
    }
    close(fifo1);

    // Wait for both children to terminate
    while (children_terminated < 2) {
        printf("Parent process proceeding\n");
        sleep(2);
    }

    printf("All child processes have terminated.\n");

    return EXIT_SUCCESS;
}

