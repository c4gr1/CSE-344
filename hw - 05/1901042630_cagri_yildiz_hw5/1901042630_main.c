#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

// Define constants for path maximum length and buffer size
#define PATH_MAX 4096
#define BUFFER_SIZE 1024

// Structure to hold file information
typedef struct {
    int src_fd;               // Source file descriptor
    int dest_fd;              // Destination file descriptor
    char src_name[PATH_MAX];  // Source file path
    char dest_name[PATH_MAX]; // Destination file path
} file_info;

// Structure to manage the shared buffer and synchronization mechanisms
typedef struct {
    file_info *buffer;        // Buffer to hold file information
    int buffer_size;          // Size of the buffer
    int in;                   // Input index for buffer
    int out;                  // Output index for buffer
    int count;                // Count of items in the buffer
    int done;                 // Flag to indicate if processing is done
    pthread_mutex_t mutex;    // Mutex to protect buffer access
    pthread_cond_t cond_full; // Condition variable to signal buffer is not full
    pthread_cond_t cond_empty;// Condition variable to signal buffer is not empty
    pthread_barrier_t barrier; // Barrier for synchronization
} shared_buffer;

// Global variables for tracking file statistics and total bytes copied
int regular_files = 0;
int fifo_files = 0;
int directories = 0;
long total_bytes_copied = 0;

// Function declarations
void *manager(void *arg);
void *worker(void *arg);
void print_statistics(int num_workers, int buffer_size, struct timespec start, struct timespec end);
void copy_file(file_info *info);
void traverse_directory(const char *src_dir, const char *dest_dir, shared_buffer *buffer);
void handle_signal(int signal);
void clean_up();

// Structure to hold manager arguments
typedef struct {
    char src_dir[PATH_MAX];   // Source directory path
    char dest_dir[PATH_MAX];  // Destination directory path
    shared_buffer *buffer;    // Pointer to shared buffer
} manager_args;

// Global variables for shared buffer and worker threads
shared_buffer buffer;
pthread_t *worker_threads;
int num_workers;
volatile sig_atomic_t sigint_received = 0;

// Function to clean up resources
void clean_up() {
    if (buffer.buffer) {
        free(buffer.buffer);
    }
    if (worker_threads) {
        free(worker_threads);
    }
    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.cond_full);
    pthread_cond_destroy(&buffer.cond_empty);
    pthread_barrier_destroy(&buffer.barrier);
}

// Signal handler for SIGINT
void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nSignal received. Cleaning up and exiting...\n");
        sigint_received = 1;
        pthread_mutex_lock(&buffer.mutex);
        buffer.done = 1;
        pthread_cond_broadcast(&buffer.cond_empty);
        pthread_cond_broadcast(&buffer.cond_full);
        pthread_mutex_unlock(&buffer.mutex);
    }
}


int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 5) {
        printf("Usage: %s <buffer size> <number of workers> <source dir> <destination dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    int buffer_size = atoi(argv[1]);
    num_workers = atoi(argv[2]);
    char *src_dir = argv[3];
    char *dest_dir = argv[4];

    // Initialize the shared buffer
    buffer.buffer_size = buffer_size;
    buffer.buffer = malloc(buffer_size * sizeof(file_info));
    buffer.in = 0;
    buffer.out = 0;
    buffer.count = 0;
    buffer.done = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.cond_full, NULL);
    pthread_cond_init(&buffer.cond_empty, NULL);
    pthread_barrier_init(&buffer.barrier, NULL, num_workers + 1);  // Initialize barrier

    // Start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Set up arguments for the manager thread
    manager_args args;
    strncpy(args.src_dir, src_dir, sizeof(args.src_dir) - 1);
    strncpy(args.dest_dir, dest_dir, sizeof(args.dest_dir) - 1);
    args.buffer = &buffer;

    // Create the manager thread
    pthread_t manager_thread;
    pthread_create(&manager_thread, NULL, manager, (void *)&args);

    // Create worker threads
    worker_threads = malloc(num_workers * sizeof(pthread_t));
    for (int i = 0; i < num_workers; i++) {
        pthread_create(&worker_threads[i], NULL, worker, (void *)&buffer);
    }

    // Set up signal handler for SIGINT
    signal(SIGINT, handle_signal);

    // Wait for the manager thread to complete
    pthread_join(manager_thread, NULL);

    // Signal worker threads that the manager is done producing
    pthread_mutex_lock(&buffer.mutex);
    buffer.done = 1;
    pthread_cond_broadcast(&buffer.cond_empty);
    pthread_cond_broadcast(&buffer.cond_full);
    pthread_mutex_unlock(&buffer.mutex);

    // Wait for all worker threads to complete
    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    print_statistics(num_workers, buffer_size, start, end);

    // Clean up resources
    clean_up();
    return 0;
}

void *manager(void *arg) {
    manager_args *args = (manager_args *)arg;
    // Traverse the source directory and fill the buffer with file information
    traverse_directory(args->src_dir, args->dest_dir, args->buffer);

    // Signal worker threads that the manager is done producing
    pthread_mutex_lock(&args->buffer->mutex);
    args->buffer->done = 1;
    pthread_cond_broadcast(&args->buffer->cond_empty);
    pthread_cond_broadcast(&args->buffer->cond_full);
    pthread_mutex_unlock(&args->buffer->mutex);

    pthread_barrier_wait(&args->buffer->barrier);  // Wait for all threads to reach the barrier
    pthread_exit(NULL);
}

void traverse_directory(const char *src_dir, const char *dest_dir, shared_buffer *buffer) {
    DIR *dir;
    struct dirent *entry;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    // Open the source directory
    if ((dir = opendir(src_dir)) == NULL) {
        perror("opendir");
        return;
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        if (sigint_received) {
            closedir(dir);
            return;
        }
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full paths for source and destination
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (entry->d_type == DT_REG) {
            // Handle regular files
            int src_fd = open(src_path, O_RDONLY);
            if (src_fd < 0) {
                perror("open src");
                continue;
            }
            int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (dest_fd < 0) {
                perror("open dest");
                close(src_fd);
                continue;
            }

            // Lock the buffer and add file information
            pthread_mutex_lock(&buffer->mutex);
            while (buffer->count == buffer->buffer_size) {
                pthread_cond_wait(&buffer->cond_full, &buffer->mutex);
            }

            file_info info = {src_fd, dest_fd, "", ""};
            strncpy(info.src_name, src_path, sizeof(info.src_name) - 1);
            strncpy(info.dest_name, dest_path, sizeof(info.dest_name) - 1);
            buffer->buffer[buffer->in] = info;
            buffer->in = (buffer->in + 1) % buffer->buffer_size;
            buffer->count++;

            // Signal that the buffer is not empty
            pthread_cond_signal(&buffer->cond_empty);
            pthread_mutex_unlock(&buffer->mutex);
        } else if (entry->d_type == DT_DIR) {
            // Handle directories
            struct stat st = {0};
            if (stat(dest_path, &st) == -1) {
                mkdir(dest_path, 0755);
            }
            directories++;
            traverse_directory(src_path, dest_path, buffer);
        }
    }

    closedir(dir);
}


void *worker(void *arg) {
    shared_buffer *buffer = (shared_buffer *)arg;
    while (1) {
        // Lock the buffer and wait if it's empty
        pthread_mutex_lock(&buffer->mutex);
        while (buffer->count == 0 && !buffer->done) {
            pthread_cond_wait(&buffer->cond_empty, &buffer->mutex);
        }
        if (buffer->count == 0 && buffer->done) {
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }

        // Get file information from the buffer
        file_info info = buffer->buffer[buffer->out];
        buffer->out = (buffer->out + 1) % buffer->buffer_size;
        buffer->count--;

        // Signal that the buffer is not full
        pthread_cond_signal(&buffer->cond_full);
        pthread_mutex_unlock(&buffer->mutex);

        // Copy the file
        copy_file(&info);
    }

    pthread_barrier_wait(&buffer->barrier);  // Wait for all threads to reach the barrier
    pthread_exit(NULL);
}

void copy_file(file_info *info) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(info->src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(info->dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write");
            break;
        }
        total_bytes_copied += bytes_written;
    }

    // Close file descriptors
    close(info->src_fd);
    close(info->dest_fd);
    regular_files++;
}


void print_statistics(int num_workers, int buffer_size, struct timespec start, struct timespec end) {
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long milliseconds = (seconds * 1000) + (nanoseconds / 1000000);
    long minutes = seconds / 60;
    seconds = seconds % 60;

    printf("\n---------------STATISTICS--------------------\n");
    printf("Consumers: %d - Buffer Size: %d\n", num_workers, buffer_size);
    printf("Number of Regular Files: %d\n", regular_files);
    printf("Number of FIFO Files: %d\n", fifo_files);
    printf("Number of Directories: %d\n", directories);
    printf("TOTAL BYTES COPIED: %ld\n", total_bytes_copied);
    printf("TOTAL TIME: %02ld:%02ld.%03ld (min:sec.mili)\n", minutes, seconds, milliseconds);
}

