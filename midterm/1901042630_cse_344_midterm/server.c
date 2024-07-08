#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>


#define MAX_CLIENTS 10
#define SERVER_FIFO_TEMPLATE "/tmp/server_fifo_%d"


int client_count = 0;
char client_names[MAX_CLIENTS][256]; // Array to store unique client names


// Function declarations
extern int client_count;
extern void handle_client(int client_pipe);
extern void cleanup();
extern void setup_server_directory(const char *dirname);
extern int create_pipe(const char *pipe_name);

sem_t sem_client_count; // Semaphore for client count
sem_t sem_file_access; // Semaphore for file access operations

void init_semaphores() {
    if (sem_init(&sem_client_count, 0, 1) != 0) {
        perror("Semaphore init failed");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_file_access, 0, 1) != 0) {
        perror("Semaphore init failed");
        exit(EXIT_FAILURE);
    }
}

void cleanup_semaphores() {
    sem_destroy(&sem_client_count);
    sem_destroy(&sem_file_access);
}


void list_files(char* response, int resp_size) {
   DIR *d;
   struct dirent *dir;
   d = opendir("."); // Open the current directory
   if (d) {
       strcpy(response, "Directory contents:\n");
       while ((dir = readdir(d)) != NULL) {
           // Ensure we do not overflow the response buffer
           if (strlen(response) + strlen(dir->d_name) + 2 < resp_size) {
               strcat(response, dir->d_name);
               strcat(response, "\n");
           } else {
               // Not enough space in buffer to add more file names
               strcat(response, "...\n");
               break;
           }
       }
       closedir(d);
   } else {
       snprintf(response, resp_size, "Failed to list directory contents: %s\n", strerror(errno));
   }
}


void read_file(const char* filename, int line_number, char* response, int resp_size) {
   
   sem_post(&sem_file_access);
   FILE *file = fopen(filename, "r");
   if (file == NULL) {
       snprintf(response, resp_size, "Error: Unable to open file '%s'.\n", filename);
       return;
   }

   char line[1024];
   int current_line = 1;
   if (line_number > 0) {
       // Fetching a specific line
       while (fgets(line, sizeof(line), file) != NULL) {
           if (current_line == line_number) {
               snprintf(response, resp_size, "Line %d: %s", line_number, line);
               fclose(file);
               return;
           }
           current_line++;
       }
       snprintf(response, resp_size, "Error: Line %d not found in '%s'.\n", line_number, filename);
   } else {
       // Reading the whole file
       strcpy(response, "File contents:\n");
       while (fgets(line, sizeof(line), file) != NULL) {
           // Ensure we do not overflow the response buffer
           if (strlen(response) + strlen(line) < resp_size) {
               strcat(response, line);
           } else {
               // Not enough space in buffer to add more lines
               strcat(response, "...\n");
               break;
           }
       }
   }
   sem_post(&sem_file_access);
   fclose(file);
}

void write_to_file(const char* filename, int line_number, const char* text, char* response, int resp_size) {

	sem_wait(&sem_file_access);
   // File operation code...

   // Open file for reading and writing, create it if it does not exist
   FILE *file = fopen(filename, "r+");
   if (file == NULL) {
       file = fopen(filename, "w+"); // Try to create the file if it doesn't exist
       if (file == NULL) {
           snprintf(response, resp_size, "Error: Unable to open or create file '%s'.\n", filename);
           return;
       }
   }

   // Move to the correct line or to the end of the file
   char line[1024];
   int current_line = 1;
   long last_pos = ftell(file);
   if (line_number > 0) {
       while (fgets(line, sizeof(line), file) != NULL) {
           if (current_line == line_number) {
               fseek(file, last_pos, SEEK_SET); // Move back to the beginning of the line
               break;
           }
           last_pos = ftell(file);
           current_line++;
       }
       if (current_line < line_number) {
           // If the file has fewer lines than line_number, pad with newlines
           while (current_line < line_number) {
               fputs("\n", file);
               current_line++;
           }
       }
   } else {
       // If no line number is specified, append to the end of the file
       fseek(file, 0, SEEK_END);
   }

   // Write the provided text to the file
   fprintf(file, "%s\n", text);

   // Response to client
   snprintf(response, resp_size, "Text written to '%s'.\n", filename);
   sem_post(&sem_file_access);
   fclose(file);
}


void download_file(const char* filename, int client_pipe) {
   FILE *file = fopen(filename, "rb"); // Open the file in binary mode to handle all file types
   if (!file) {
       char response[1024];
       snprintf(response, sizeof(response), "Error: Unable to open file '%s' for download.\n", filename);
       write(client_pipe, response, strlen(response));
       return;
   }

   // Inform client that file download is starting
   char buffer[4096];
   snprintf(buffer, sizeof(buffer), "Starting file download for '%s'...\n", filename);
   write(client_pipe, buffer, strlen(buffer));

   // Read the file in chunks and send it to the client
   int bytes_read;
   while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
       if (write(client_pipe, buffer, bytes_read) != bytes_read) {
           fprintf(stderr, "Error: Failed to send file data to client.\n");
           break;
       }
   }

   // Check for read error
   if (ferror(file)) {
       snprintf(buffer, sizeof(buffer), "Error: Failed to read file '%s'.\n", filename);
       write(client_pipe, buffer, strlen(buffer));
   } else {
       snprintf(buffer, sizeof(buffer), "File download completed successfully.\n");
       write(client_pipe, buffer, strlen(buffer));
   }

   fclose(file);
}

void upload_file(const char* filename, int client_pipe) {
   FILE *file = fopen(filename, "wb"); // Open the file in binary mode for writing
   if (!file) {
       char response[1024];
       snprintf(response, sizeof(response), "Error: Unable to create or open file '%s'.\n", filename);
       write(client_pipe, response, strlen(response));
       return;
   }

   char buffer[4096];
   snprintf(buffer, sizeof(buffer), "Ready to receive file '%s'. Send data.\n", filename);
   write(client_pipe, buffer, strlen(buffer));

   // Read data from the client pipe and write to the file
   int bytes_read;
   while ((bytes_read = read(client_pipe, buffer, sizeof(buffer))) > 0) {
       if (fwrite(buffer, 1, bytes_read, file) != bytes_read) {
           fprintf(stderr, "Error writing to file: %s\n", filename);
           break;
       }
   }

   if (bytes_read < 0) {
       snprintf(buffer, sizeof(buffer), "Error: Failed to read data from client for file '%s'.\n", filename);
       write(client_pipe, buffer, strlen(buffer));
   } else {
       snprintf(buffer, sizeof(buffer), "File '%s' uploaded successfully.\n", filename);
       write(client_pipe, buffer, strlen(buffer));
   }

   fclose(file);
}

void archive_files(const char* tarname, int client_pipe) {
   char command[1024];
   char response[2048];

   // Construct the tar command to create an archive of the current directory
   snprintf(command, sizeof(command), "tar -cf %s *", tarname);

   // Execute the tar command
   int result = system(command);
   if (result != 0) {
       snprintf(response, sizeof(response), "Error: Failed to create archive '%s'.\n", tarname);
       write(client_pipe, response, strlen(response));
   } else {
       snprintf(response, sizeof(response), "Archive created successfully: '%s'\n", tarname);
       write(client_pipe, response, strlen(response));
   }
}

void kill_server() {
   // Add any cleanup needed before killing the server
   printf("Server is shutting down...\n");
   exit(0); // Exit the application
}


void help(int client_pipe) {
   const char* help_message = 
       "Available commands are:\n"
       "help\n"
       "   Display the list of possible client requests.\n"
       "list\n"
       "   Lists all files in the server's directory.\n"
       "readF <file> <line #>\n"
       "   Displays the specified line of the file. If no line number is given, the whole file is displayed.\n"
       "writeT <file> <line #> <string>\n"
       "   Writes the content of 'string' to the specified line number of the file. If no line number is given, writes to the end of the file.\n"
       "upload <file>\n"
       "   Uploads the specified file from the client to the server's directory.\n"
       "download <file>\n"
       "   Downloads the specified file from the server's directory to the client.\n"
       "archServer <fileName>.tar\n"
       "   Archives all the files in the server's directory into the specified tar file.\n"
       "killServer\n"
       "   Sends a request to the server to terminate gracefully.\n"
       "quit\n"
       "   Disconnects the client from the server and closes the session.\n";

   // Send the help message to the client
   write(client_pipe, help_message, strlen(help_message));
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dirname> <max_clients>\n", argv[0]);
        return 1;
    }

    const char* dirname = argv[1];
    int max_clients = atoi(argv[2]);
    setup_server_directory(dirname);
    if (chdir(dirname) != 0) {
        perror("Failed to change directory");
        return 1;
    }

    char server_fifo_name[256];
    snprintf(server_fifo_name, sizeof(server_fifo_name), SERVER_FIFO_TEMPLATE, getpid());

    unlink(server_fifo_name);
    if (mkfifo(server_fifo_name, 0666) != 0) {
        perror("Failed to create server FIFO");
        return 1;
    }

    int server_fifo = open(server_fifo_name, O_RDONLY | O_NONBLOCK);
    if (server_fifo == -1) {
        perror("Failed to open server FIFO");
        return 1;
    }

    printf("> Server Started PID %d…\n", getpid());
    printf(">> waiting for clients...\n");

    init_semaphores();

    while (1) {
        char buffer[1024];
        int numBytes = read(server_fifo, buffer, sizeof(buffer) - 1);
        if (numBytes > 0) {
            buffer[numBytes] = '\0'; // Null-terminate the string

            // Extracting the FIFO name properly, assuming it ends with a newline
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';  // Trim the newline

            if (strncmp(buffer, "/tmp/client_fifo_", 17) == 0) {
                if (client_count < max_clients) {
                    int client_fifo_fd = create_pipe(buffer);
                    if (client_fifo_fd != -1) {
                        printf(">> Client PID %d connected as “%s”\n", getpid(), buffer);
                        pid_t pid = fork();
                        if (pid == 0) {
                            close(server_fifo);
                            handle_client(client_fifo_fd);
                            exit(0);
                        } else if (pid > 0) {
                            sem_wait(&sem_client_count);
                            client_count++;
                            sem_post(&sem_client_count);
                            close(client_fifo_fd); // Parent doesn't communicate directly with clients
                        }
                    } else {
                        printf(">> Failed to create or open client FIFO.\n");
                    }
                } else {
                    printf(">> Max client count reached.\n");
                }
            } else {
                printf(">> Received non-FIFO message or command: '%s'\n", buffer);
            }
        }
    }

    return 0;
}


void handle_client(int client_pipe) {
   char command[1024];
   char response[2048];
   int nbytes;
   
   // Set the pipe to non-blocking mode
	int flags = fcntl(client_pipe, F_GETFL, 0);
	fcntl(client_pipe, F_SETFL, flags | O_NONBLOCK);

    while ((nbytes = read(client_pipe, command, sizeof(command))) > 0) {
        command[nbytes] = '\0'; // Ensure null termination
        printf(">> Client PID %d command: '%s'\n", getpid(), command); // Print each command


	   printf("Processed command: '%s'\n", command);

       // Process the command
       if (strncmp(command, "help", 4) == 0) {
          help(client_pipe);
       } else if (strncmp(command, "list", 4) == 0) {
           list_files(response, sizeof(response));
           write(client_pipe, response, strlen(response)); // Send list output back to client
       } else if (strncmp(command, "readF", 5) == 0) {
           char filename[256];
           int line_number;
           sscanf(command + 6, "%s %d", filename, &line_number);
           read_file(filename, line_number, response, sizeof(response));
           write(client_pipe, response, strlen(response));
       } else if (strncmp(command, "writeT", 6) == 0) {
           char filename[256];
           int line_number;
           char text[1024];
           sscanf(command + 7, "%s %d %[^\n]", filename, &line_number, text);
           write_to_file(filename, line_number, text, response, sizeof(response));
           write(client_pipe, response, strlen(response));
       } else if (strncmp(command, "download", 8) == 0) {
           char filename[256];
           sscanf(command + 9, "%s", filename);
           download_file(filename, client_pipe);
       } else if (strncmp(command, "upload", 6) == 0) {
           char filename[256];
           sscanf(command + 7, "%s", filename);
           upload_file(filename, client_pipe);
       } else if (strncmp(command, "archServer", 10) == 0) {
           char tarname[256];
           sscanf(command + 11, "%s", tarname);
           archive_files(tarname, client_pipe);
       } else if (strncmp(command, "killServer", 10) == 0) {
           kill_server();
       } else if (strncmp(command, "quit", 4) == 0) {
           printf("Client requested to quit.\n");
           break; // Exit the loop and close this client's connection
       } else {
           snprintf(response, sizeof(response), "Unknown command: %s\n", command);
           write(client_pipe, response, strlen(response));
       }

   }

   if (nbytes < 0) {
       perror("Failed to read from client FIFO");
   }

   close(client_pipe); // Close the client pipe when done
   printf("Client disconnected.\n");
}



void cleanup() {
    cleanup_semaphores(); // Clean up semaphores
    printf("Server shutting down...\n");
    // Perform cleanup, such as unlinking the server FIFO
    unlink("/tmp/server_fifo"); // Adjust as necessary based on your actual server FIFO path
    exit(0);
}

void setup_server_directory(const char *dirname) {
   printf("Setting up server directory...\n");
   if (mkdir(dirname, 0755) == -1) {
       if (errno != EEXIST) {
           perror("Failed to create directory");
           exit(EXIT_FAILURE);
       }
   }
}

int create_pipe(const char *pipe_name) {
   printf("Creating pipe: %s\n", pipe_name);
   unlink(pipe_name);
   if (mkfifo(pipe_name, 0666) != 0) {
       perror("Failed to create client FIFO");
       return -1;
   }
   int pipe_fd = open(pipe_name, O_RDWR);
   if (pipe_fd == -1) {
       perror("Failed to open client FIFO");
       return -1;
   }
   return pipe_fd;
}
