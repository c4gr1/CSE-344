#include "common.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>

/*
 * Global socket descriptor for the client.
 * Side effects:
 * - This is used by various functions to send and receive data over the network.
 */
int sock;

/*
 * Flag to keep the client running.
 * Side effects:
 * - This is used to control the main loop of the client program.
 */
int keep_running = 1;

/*
 * Signal handler for orderly shutdown of the client.
 * Side effects:
 * - Sends a cancellation message over the network.
 * - Closes the network socket.
 * - Exits the program.
 * - Uses standard input/output functions which may block execution.
 */
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("Client: Order cancelled by user\n");
        char message[] = "Order cancelled";
        send(sock, message, strlen(message), 0);
        close(sock);
        exit(0);
    }
}

/*
 * Handle EOF (End of File) from standard input.
 * Side effects:
 * - Sends a cancellation message over the network.
 * - Closes the network socket.
 * - Exits the program.
 * - Uses standard input/output functions which may block execution.
 */
void handle_eof() {
    printf("Client: Order cancelled by EOF\n");
    char message[] = "Order cancelled";
    send(sock, message, strlen(message), 0);
    close(sock);
    exit(0);
}


int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [server_ip] [numberOfClients] [townWidth] [townHeight]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client Step 1: Connecting to server...\n");
    const char *server_ip = argv[1];
    int num_clients = atoi(argv[2]);
    int town_width = atoi(argv[3]);
    int town_height = atoi(argv[4]);
    struct sockaddr_in server;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Connect to the server once
    printf("Client Step 2: Creating socket...\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    printf("Client Step 3: Converting IP address...\n");
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(8000);

    printf("Client Step 4: Connecting to server...\n");
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Client Step 5: Connected to server. Starting to send messages...\n");

    for (int i = 0; i < num_clients; i++) {
        // Generate random position within the town
        float posX = (float)(rand() % (town_width + 1));
        float posY = (float)(rand() % (town_height + 1));
        char message[256];
        snprintf(message, sizeof(message), "Order from client %d at position (%.2f, %.2f)", i + 1, posX, posY);
        send(sock, message, strlen(message), 0);
        printf("Message sent to server from client %d\n", i + 1);

        // Receive response from server
        char server_reply[2000];
        int reply_len = recv(sock, server_reply, 1999, 0);
        if (reply_len < 0) {
            perror("recv failed");
            close(sock);
            exit(EXIT_FAILURE);
        }
        server_reply[reply_len] = '\0'; // Null-terminate the received string
        printf("Message from server: %s\n", server_reply);

        // Check for EOF
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(0, &readfds); // Add stdin (fd 0) to the set
        struct timeval timeout = {2, 0}; // Timeout for select (2 seconds)

        int activity = select(1, &readfds, NULL, NULL, &timeout);

        if (activity == -1) {
            perror("select error");
            break;
        } else if (activity == 0) {
            // Timeout occurred, continue waiting
            continue;
        }

        if (FD_ISSET(0, &readfds)) {
            // Check for EOF (Ctrl+D)
            char buf[1];
            if (read(0, buf, 1) == 0) {
                handle_eof();
            }
        }
    }

    printf("Client Step 6: All messages sent. Closing connection...\n");
    close(sock);
    printf("Client Step 7: Connection closed. All customers served. Writing log file...\n");

    return 0;
}
