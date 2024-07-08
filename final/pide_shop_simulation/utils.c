#include "utils.h"
#include <time.h>

// Prints an error message and terminates the program
void handle_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE); // Exit calling program
}

// Logging function
void log_message(const char *message) {
    time_t now;
    time(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] %s\n", timestamp, message);
    FILE *log_file = fopen("pide_shop_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", timestamp, message);
        fclose(log_file);
    } else {
        perror("Failed to open log file");
    }
}
