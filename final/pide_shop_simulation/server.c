#include "common.h"  // Common definitions and declarations shared across multiple files
#include "protocol.h" // Protocol definitions for message types, order statuses, and error codes
#include "utils.h"    // Utility functions for logging and error handling
#include <stdio.h>    // Standard I/O operations
#include <stdlib.h>   // Standard library functions such as memory allocation
#include <string.h>   // String handling functions
#include <unistd.h>   // POSIX API for system calls (e.g., fork, exec)
#include <sys/socket.h> // Definitions for sockets
#include <netinet/in.h> // Structures for internet addresses
#include <pthread.h>    // POSIX threads for multithreading
#include <signal.h>     // Signal handling functions
#include <arpa/inet.h>  // Definitions for internet operations
#include <math.h>       // Mathematical functions

/*
 * Side effects:
 * - The function creates and binds a socket, which consumes system resources.
 * - It allocates memory dynamically for new socket descriptors.
 * - It creates new threads, which could lead to a high number of concurrent threads if many clients connect simultaneously.
 * - Logs various messages which could affect performance if logging is intensive.
 * - Uses standard input/output functions which may block execution.
 * - It calls perror and log_message functions, affecting program state or output.
 */

// Function to start the server and handle client connections
void start_server(const char *ip_address, int port);

// Signal handler to gracefully shut down the server
void signal_handler(int sig);

// Function to write final log entries before shutting down
void write_log_file();

// Function to cancel all ongoing orders
void cancel_all_orders();

// Function to calculate delivery time based on distance and velocity
float calculate_delivery_time(float x, float y, float velocity);

// External function declarations to start various components
extern void start_cooks(int num_cooks);
extern void start_delivery_system(int num_deliveries, int delivery_speed, int width, int height);
extern void start_manager();
extern void cancel_order();

// Function prototypes for internal use
void *client_handler(void *socket);  // Handle individual client connections
void signal_cooks();                 // Signal cooks to start preparing orders
void signal_delivery_personnel();    // Signal delivery personnel for delivery
void notify_manager();               // Notify manager about order status
void manager_receive_order();        // Manager receives and processes orders


void start_server(const char *ip_address, int port) {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;

    // Step 5: Creating socket
    printf("Server Step 5: Creating socket...\n");
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        log_message("Could not create socket");
        return;
    }
    log_message("Socket created");

    // Step 6: Preparing sockaddr_in structure
    printf("Server Step 6: Preparing sockaddr_in structure...\n");
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_port = htons(port);

    // Step 7: Binding socket
    printf("Server Step 7: Binding socket...\n");
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return;
    }
    log_message("bind done");

    // Step 8: Listening for connections
    printf("Server Step 8: Listening for connections...\n");
    if (listen(socket_desc, 3) < 0) {
        perror("listen failed. Error");
        return;
    }

    log_message("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    // Continuously accept incoming connections
    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))) {
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }
        log_message("Connection accepted");

        // Allocate memory for the new socket descriptor
        pthread_t thread_id;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        // Step 9: Creating client handler thread
        printf("Server Step 9: Creating client handler thread...\n");
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) < 0) {
            perror("could not create thread");
            free(new_sock);
            continue;
        }

        pthread_detach(thread_id); // Detach the thread to handle the client independently
        log_message("Handler assigned");
    }

    close(socket_desc); // Close the socket when done
}

/* 
 * Function to handle each client connection.
 * Side effects:
 * - Frees allocated memory for the socket.
 * - Logs various messages which could affect performance if logging is intensive.
 * - Uses standard input/output functions which may block execution.
 * - It calls perror and log_message functions, affecting program state or output.
 */
void *client_handler(void *socket) {
    int sock = *((int *)socket);
    free(socket);

    char buffer[1024];
    int read_size;

    log_message("New client connected");

    // Receive messages from the client
    while ((read_size = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0'; // Null-terminate the received string
        float posX, posY;
        sscanf(buffer, "Order from client %*d at position (%f, %f)", &posX, &posY);

        if (strncmp(buffer, "Order cancelled", 15) == 0) {
            cancel_order();
            log_message("Order cancelled by client");
            continue;
        }

        float delivery_time = calculate_delivery_time(posX, posY, 0.5);

        log_message("Received order from client");
        snprintf(buffer, sizeof(buffer), "Order processed by server. Estimated delivery time: %.2f minutes", delivery_time);
        send(sock, buffer, strlen(buffer), 0);
        log_message("Order processed and response sent to client");

        manager_receive_order();
        signal_cooks();
    }

    if (read_size == 0) {
        log_message("Client disconnected");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(sock); // Close the socket when done
    return NULL;
}

/* 
 * Function to calculate the delivery time based on coordinates and velocity.
 * No side effects.
 */
float calculate_delivery_time(float x, float y, float velocity) {
    float distance = sqrt(x * x + y * y);
    return distance / velocity;
}

/* 
 * Signal handler for orderly shutdown of the server.
 * Side effects:
 * - Logs various messages.
 * - Calls exit() to terminate the program.
 */
void signal_handler(int sig) {
    log_message("Signal received, shutting down...");
    cancel_all_orders();
    write_log_file();
    log_message("Log file written");
    exit(0);
}

/* 
 * Function to write final log entries to a file.
 * Side effects:
 * - Logs a message indicating that the log file is being written.
 */
void write_log_file() {
    log_message("Writing final log entries...");
}

/* 
 * Function to cancel all orders due to an emergency (e.g., shop fire).
 * Side effects:
 * - Logs a message indicating that all orders are being cancelled.
 * - Calls cancel_order() to handle the order cancellation process.
 */
void cancel_all_orders() {
    log_message("Cancelling all orders due to shop fire");
    cancel_order();
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [IP address] [CookThreadPoolSize] [DeliveryPoolSize] [Speed (m/min)]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Server Step 1: Parsing arguments...\n");
    const char *ip_address = argv[1];
    int cook_thread_pool_size = atoi(argv[2]);
    int delivery_thread_pool_size = atoi(argv[3]);
    float delivery_speed = atof(argv[4]);
    int port = 8000; // Use port 8000

    printf("Server Step 2: Setting up signal handlers...\n");
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE signal to prevent crashes on broken pipe

    printf("Server Step 3: Starting thread pools...\n");
    printf("Starting cook threads...\n");
    start_cooks(cook_thread_pool_size);
    printf("Cook threads started...\n");
    
    printf("Starting delivery threads...\n");
    start_delivery_system(delivery_thread_pool_size, delivery_speed, cook_thread_pool_size, delivery_thread_pool_size);
    printf("Delivery threads started...\n");
    
    printf("Starting manager...\n");
    start_manager();
    printf("Manager started...\n");

    printf("Server Step 4: Starting server...\n");
    start_server(ip_address, port);

    return 0;
}
