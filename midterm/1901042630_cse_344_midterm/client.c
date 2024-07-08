#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_FIFO_TEMPLATE "/tmp/server_fifo_%d"
#define CLIENT_FIFO_TEMPLATE "/tmp/client_fifo_%d"
#define CLIENT_FIFO_NAME_LEN 256

void connect_server(const char *server_fifo, const char *client_fifo);
void send_command(const char *server_fifo, const char *message);
void read_response(const char *client_fifo);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ServerPID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_pid = atoi(argv[1]);
    char server_fifo_name[256];
    char client_fifo_name[CLIENT_FIFO_NAME_LEN];
    
    snprintf(server_fifo_name, sizeof(server_fifo_name), SERVER_FIFO_TEMPLATE, server_pid);
    snprintf(client_fifo_name, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, getpid());

    if (mkfifo(client_fifo_name, 0666) == -1) {
        perror("Client FIFO creation failed");
        exit(EXIT_FAILURE);
    }

    connect_server(server_fifo_name, client_fifo_name);

    char command[256];
    printf("Enter command (type 'exit' to quit): ");
    while (fgets(command, sizeof(command), stdin) && strncmp(command, "exit", 4) != 0) {
        send_command(server_fifo_name, command);
        read_response(client_fifo_name);
    }

    unlink(client_fifo_name);
    return 0;
}

void connect_server(const char *server_fifo, const char *client_fifo) {
    int server_fd = open(server_fifo, O_WRONLY);
    if (server_fd == -1) {
        perror("Failed to open server FIFO");
        exit(EXIT_FAILURE);
    }

    // Notify server of new client FIFO
    char message[1024];
    snprintf(message, sizeof(message), "%s\n", client_fifo); // Ensure the message ends with a newline
    if (write(server_fd, message, strlen(message)) != strlen(message)) {
        perror("Failed to write to server FIFO");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    close(server_fd);
}

void send_command(const char *server_fifo, const char *message) {
    int server_fd = open(server_fifo, O_WRONLY);
    if (server_fd == -1) {
        perror("Failed to open server FIFO");
        exit(EXIT_FAILURE);
    }

    char full_message[1024]; // Yeterli büyüklükte bir tampon
    snprintf(full_message, sizeof(full_message), "%s\n", message); // Mesaja newline ekleyin

    if (write(server_fd, full_message, strlen(full_message)) != strlen(full_message)) {
        perror("Failed to write to server FIFO");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    close(server_fd);
}


void read_response(const char *client_fifo) {
    int client_fd = open(client_fifo, O_RDONLY);
    if (client_fd == -1) {
        perror("Failed to open client FIFO");
        exit(EXIT_FAILURE);
    }

    char response[1024];
    ssize_t numBytes = read(client_fd, response, sizeof(response) - 1);
    if (numBytes == -1) {
        perror("Failed to read from client FIFO");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    response[numBytes] = '\0';
    printf("Server response:\n%s\n", response);
    close(client_fd);
}
