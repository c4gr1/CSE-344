#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h> // For exit

// Logging macro
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) // Do nothing in non-debug mode
#endif

// Error handling
void handle_error(const char *message);
void log_message(const char *message);

#endif // UTILS_H
