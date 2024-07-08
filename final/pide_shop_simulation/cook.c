#include "common.h"
#include "protocol.h"
#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <unistd.h>

// Define structure for cook
typedef struct {
    pthread_t thread;
    int id;
} Cook;

typedef struct {
    int num_aparatus; // Number of available oven aparatus
    int num_meals;    // Number of meals currently in the oven
    pthread_mutex_t mutex;
    pthread_cond_t cond_aparatus;
    pthread_cond_t cond_meals;
} Oven;

#define MAX_APARATUS 3
#define MAX_MEALS 6

// Array to hold all cooks
static Cook *cooks;
static int num_cooks;
static pthread_mutex_t cook_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cook_cond = PTHREAD_COND_INITIALIZER;
static int order_available = 0;
static int order_count = 0;

// Oven structure
static Oven oven = {MAX_APARATUS, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

// Prototype for cook thread function
void *cook_thread(void *arg);
void compute_pseudo_inverse(void);

// Initialize cooks
void start_cooks(int num_cooks_param) {
    printf("Initializing cooks...\n");
    num_cooks = num_cooks_param;
    cooks = malloc(num_cooks * sizeof(Cook));
    for (int i = 0; i < num_cooks; i++) {
        cooks[i].id = i;
        if (pthread_create(&cooks[i].thread, NULL, cook_thread, (void *)&cooks[i]) != 0) {
            perror("Failed to create cook thread");
            exit(EXIT_FAILURE);
        }
    }
    printf("Cooks initialized...\n");
}

// Thread function for each cook
void *cook_thread(void *arg) {
    Cook *cook = (Cook *)arg;

    while (1) {
        pthread_mutex_lock(&cook_mutex);
        while (!order_available) {
            pthread_cond_wait(&cook_cond, &cook_mutex);
        }
        order_available--;
        int order_id = ++order_count;
        pthread_mutex_unlock(&cook_mutex);

        char message[256];
        snprintf(message, sizeof(message), "Preparing order %d by cooker %d", order_id, cook->id);
        log_message(message);

        // Simulate cooking by computing pseudo-inverse
        compute_pseudo_inverse();

        // Simulate oven usage
        pthread_mutex_lock(&oven.mutex);
        while (oven.num_aparatus == 0) {
            pthread_cond_wait(&oven.cond_aparatus, &oven.mutex);
        }
        oven.num_aparatus--;
        while (oven.num_meals == MAX_MEALS) {
            pthread_cond_wait(&oven.cond_meals, &oven.mutex);
        }
        oven.num_meals++;
        oven.num_aparatus++;
        pthread_cond_signal(&oven.cond_aparatus);
        pthread_mutex_unlock(&oven.mutex);

        snprintf(message, sizeof(message), "Order %d is being cooked by cooker %d", order_id, cook->id);
        log_message(message);

        sleep(1); // Simulate cooking time in the oven

        pthread_mutex_lock(&oven.mutex);
        oven.num_meals--;
        pthread_cond_signal(&oven.cond_meals);
        pthread_mutex_unlock(&oven.mutex);

        snprintf(message, sizeof(message), "Order %d is cooked by cooker %d", order_id, cook->id);
        log_message(message);

        // Notify manager that the order is ready
        notify_manager();
    }

    return NULL;
}

// Function to compute the pseudo-inverse of a 30x40 matrix with complex elements
void compute_pseudo_inverse(void) {
    int m = 30, n = 40;
    double complex A[m][n];
    double complex B[n][m];

    // Initialize matrix A with example values
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = 1.0 + 1.0 * I; // Example values
        }
    }

    // Simulate computation of the pseudo-inverse using Singular Value Decomposition (SVD)
    svd_pseudo_inverse(m, n, A, B);
}

// Singular Value Decomposition (SVD) based pseudo-inverse computation
void svd_pseudo_inverse(int m, int n, double complex A[m][n], double complex B[n][m]) {

    sleep(2); // Simulate time taken for SVD computation
}

// Function to signal cooks when an order is available
void signal_cooks() {
    pthread_mutex_lock(&cook_mutex);
    order_available++;
    pthread_cond_signal(&cook_cond);
    pthread_mutex_unlock(&cook_mutex);
}
