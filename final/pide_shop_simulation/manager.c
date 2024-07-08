#include "common.h"
#include "protocol.h"
#include "utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Function prototypes for internal use
 */
void *manager_thread(void *arg);
void signal_cooks();
void signal_delivery_personnel();

/*
 * Global variables for managing order states and synchronization
 * Side effects:
 * - These variables are shared between the manager and other threads to manage orders and synchronize their processing.
 */
static pthread_mutex_t manager_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t manager_cond = PTHREAD_COND_INITIALIZER;
static int order_received = 0;
static int order_ready = 0;

/*
 * Start the manager thread
 * Side effects:
 * - Creates and detaches a new thread for the manager.
 * - Logs the initialization status.
 */
void start_manager(void) {
    printf("Initializing manager...\n");
    pthread_t thread;
    if (pthread_create(&thread, NULL, manager_thread, NULL) != 0) {
        perror("Failed to create manager thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(thread);
    printf("Manager initialized...\n");
}

/*
 * Function representing the manager thread
 * Side effects:
 * - Continuously checks for ready orders and signals delivery personnel.
 */
void *manager_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&manager_mutex);
        while (!order_ready) {
            pthread_cond_wait(&manager_cond, &manager_mutex);
        }
        order_ready--;
        pthread_mutex_unlock(&manager_mutex);

        log_message("Manager: Order is ready for delivery");
        signal_delivery_personnel();
    }
    return NULL;
}

/*
 * Notify the manager that an order is ready
 * Side effects:
 * - Updates the condition variable to wake up the manager thread.
 */
void notify_manager() {
    pthread_mutex_lock(&manager_mutex);
    order_ready++;
    pthread_cond_signal(&manager_cond);
    pthread_mutex_unlock(&manager_mutex);
}

/*
 * Manager receives a new order
 * Side effects:
 * - Updates the condition variable to wake up the manager thread.
 * - Logs the order received message.
 */
void manager_receive_order() {
    pthread_mutex_lock(&manager_mutex);
    order_received++;
    pthread_cond_signal(&manager_cond);
    pthread_mutex_unlock(&manager_mutex);

    char message[256];
    snprintf(message, sizeof(message), "Manager received an order");
    log_message(message);
}

/*
 * Cancel all orders and notify all waiting threads
 * Side effects:
 * - Resets order counters and signals all waiting threads.
 * - Logs the cancellation message.
 */
void cancel_order() {
    pthread_mutex_lock(&manager_mutex);
    order_received = 0;
    order_ready = 0;
    pthread_cond_broadcast(&manager_cond);
    pthread_mutex_unlock(&manager_mutex);

    log_message("All orders cancelled");
}
