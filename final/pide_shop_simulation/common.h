#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>

// Constants defining the limits and operational parameters
#define MAX_COOKS 3
#define MAX_DELIVERIES 3
#define MAX_OVEN_CAPACITY 6
#define OVEN_OPENINGS 2

// Function prototypes
void log_message(const char *message);
void notify_manager();
void svd_pseudo_inverse(int m, int n, double complex A[m][n], double complex B[n][m]);

// Debugging helper macros
#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

#endif // COMMON_H
