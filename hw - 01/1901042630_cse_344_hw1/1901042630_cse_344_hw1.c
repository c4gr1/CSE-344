#include <stdio.h>      // Standard I/O operations
#include <stdlib.h>     // Standard library for memory allocation, process control, etc.
#include <string.h>     // String handling functions
#include <unistd.h>     // POSIX operating system API for system calls
#include <fcntl.h>      // File control options for open
#include <sys/wait.h>   // Declarations for waiting for process termination
#include <sys/types.h>  // Basic derived types
#include <time.h>       // Time types

#define MAX_LINE_LENGTH 256  // Define the maximum length for reading lines from a file

// Structure to represent a student with name and grade
typedef struct {
    char name[100];   // Student name
    char grade[3];    // Student grade, assuming format is like "AA", "BB", etc.
} Student;

// Enumeration for sorting criteria (by name or by grade)
typedef enum {
    SORT_BY_NAME,
    SORT_BY_GRADE
} SortCriteria;

// Enumeration for sorting order (ascending or descending)
typedef enum {
    ASCENDING,
    DESCENDING
} SortOrder;

// Global variables to hold current sorting criteria and order
// These can be changed dynamically based on user input or program logic
SortCriteria currentSortCriteria = SORT_BY_NAME; // Default sorting criteria set to name
SortOrder currentSortOrder = ASCENDING;          // Default sorting order set to ascending

/*---------------------------------------------------*/

// Creates a new file with the specified filename. If the file already exists, this function will not overwrite it.
void createFile(const char* filename);

// Adds a new student grade entry to the file. Each entry includes the student's name and grade.
void addStudentGrade(const char* filename, const char* name, const char* grade);

// Searches for a student by name in the specified file and displays the student's grade if found.
void searchStudent(const char* filename, const char* name);

// Compares two Student structures based on the current sorting criteria (name or grade) and order (ascending or descending).
int compareStudents(const void* a, const void* b);

// Reads student entries from the file, sorts them based on the current criteria and order, and prints the sorted list.
void performSorting(const char* fileName);

// Facilitates the sorting of student grades in a file based on specified criteria and order, using a separate process.
void sortAll(const char* fileName, SortCriteria criteria, SortOrder order);

// Reads lines from a file starting at a specific line number and prints a specified number of lines, using a separate process for isolation.
void processFileWithFork(const char* filename, int startLine, int numLines);

// Displays all student grades stored in the file by printing each student's name and grade.
void showAll(const char* filename);

// Lists the first 5 entries of student grades from the file, showing the student name and grade for each.
void listGrades(const char* filename);

// Lists a specific number of entries (numOfEntries) from a specified page number in the file, helping in paginated display of entries.
void listSome(const char* filename, int numOfEntries, int pageNumber);

// Displays all available commands and instructions for the program, offering guidance on how to use each command.
void displayHelp();

// Logs a message to a log file with a timestamp, aiding in debugging and tracking the execution of the program.
void logMessage(const char* message);

/*---------------------------------------------------*/


int main(){

// Buffer to store user input command.
char command[256];

// Display initial command prompts to the user.
printf("\nEnter a Command :\n");
printf("    To show all commands -> gtuStudentGrades \n");
printf("    To exit -> q \n\n");

// Main loop to continuously accept user commands until 'q' is entered.
    while (1) {
        printf("-> "); // Prompt for user input.
        
        // Read a line of input from the user.
        fgets(command, 256, stdin);

        // Remove the newline character at the end of the command.
        command[strcspn(command, "\n")] = 0;

        // Check if the user wants to quit the program.
        if(strcmp(command, "q") == 0) {
            break; // Exit the loop and terminate the program.
        }

        // Handle the 'gtuStudentGrades' command.
        if(strncmp(command, "gtuStudentGrades", 16) == 0){
            char filename[50];

            // Try to parse a filename from the command.
            if(sscanf(command,"%*s %s ", filename) == 1){
                // If a filename is provided, create the file.
                createFile(filename);
            } else if(sscanf(command,"%*s")== 0){
                // If no additional arguments are provided, display the help menu.
                displayHelp();                
            } else {
                // If the command is incorrectly formatted, display an error message.
                printf("Error parsing the gtuStudentGrades command.\n");
            }             
        } 
        // Handle the 'addStudentGrades' command.
        else if(strncmp(command, "addStudentGrade", 15) == 0) {
            char filename[50], name[100], grade[5];

            // Try to parse filename, student name, and grade from the command.
            if(sscanf(command,"%*s %s \"%[^\"]\" \"%[^\"]\"", filename, name, grade) == 3) {
                // If all arguments are successfully parsed, add the student grade.
                addStudentGrade(filename, name, grade);
            } else {
                // If the command is incorrectly formatted, display an error message.
                printf("Error parsing the addStudentGrade command.\n");
            }
        } // Handle the 'searchStudent' command for searching a student by name within a specified file.
        else if(strncmp(command, "searchStudent", 13) == 0){
            char filename[50], name[100];

            // Attempt to parse the filename and student name from the command.
            if(sscanf(command,"%*s %s \"%[^\"]\"", filename, name) == 2){
                // If parsing is successful, call the function to search for the student.
                searchStudent(filename,name);
            } else {
                // If the command is incorrectly formatted, display an error message.
                printf("Error parsing the searchStudent command.\n");
            } 
        }
        // Handle the 'sortAll' command for sorting all student grades within a specified file.
        else if(strncmp(command, "sortAll", 7) == 0) {
            char filename[50];
            char criteriaStr[10];
            char orderStr[5];

            // Attempt to parse the command for filename, criteria, and sorting order from the input.
            if(sscanf(command, "%*s %s %s %s", filename, criteriaStr, orderStr) == 3 || sscanf(command, "%*s %s ", filename) == 1 ) {
                // Initialize sorting criteria and order with default values.
                SortCriteria criteria = SORT_BY_NAME;
                SortOrder order = ASCENDING;

                // Adjust sorting criteria based on parsed input.
                if(strcmp(criteriaStr, "name") == 0) {
                    criteria = SORT_BY_NAME;
                } else if(strcmp(criteriaStr, "grade") == 0) {
                    criteria = SORT_BY_GRADE;
                } else {
                    // If criteria string doesn't match expected values, use default and notify user.
                    fprintf(stderr, "Default sort criteria. Use 'name' or 'grade'.\n");
                }

                // Adjust sorting order based on parsed input.
                if(strcmp(orderStr, "asc") == 0) {
                    order = ASCENDING;
                } else if(strcmp(orderStr, "desc") == 0) {
                    order = DESCENDING;
                } else {
                    // If order string doesn't match expected values, use default and notify user.
                    fprintf(stderr, "Default sort order. Use 'asc' or 'desc'.\n");
                }

                // Call the sortAll function with the parsed and adjusted arguments.
                sortAll(filename, criteria, order);
            } else {
                // If the command parsing fails, notify the user with an error message.
                fprintf(stderr, "Error parsing the sortAll command.\n");
            }
        } // Handle the 'showAll' command to display all student grades stored in the specified file.
        else if(strncmp(command, "showAll", 7) == 0) {
            char filename[50]; // Buffer to hold the filename.

            // Attempt to parse the filename from the command.
            if(sscanf(command, "%*s %s", filename) == 1) {
                // If successful, call the function to display all student grades.
                showAll(filename);
            } else {
                // Display an error message if the command usage is incorrect.
                fprintf(stderr, "Error: Incorrect usage of showAll.\n");
            }
        }
        // Handle the 'listGrades' command to display the first few student grades from the specified file.
        else if(strncmp(command, "listGrades", 10) == 0) {
            char filename[50]; // Buffer to hold the filename.

            // Attempt to parse the filename from the command.
            if(sscanf(command, "%*s %s", filename) == 1) {
                // If successful, call the function to list the first few student grades.
                listGrades(filename);
            } else {
                // Display an error message if the command usage is incorrect.
                fprintf(stderr, "Error: Incorrect usage of listGrades.\n");
            }
        }
        // Handle the 'listSome' command to list a specific number of student grades from a specified page number in the file.
        else if(strncmp(command, "listSome", 8) == 0) {
            char filename[50]; // Buffer to hold the filename.
            int numEntries, pageNumber; // Variables to hold the number of entries to list and the page number.

            // Attempt to parse the number of entries, page number, and filename from the command.
            if(sscanf(command, "%*s %d %d %s", &numEntries, &pageNumber, filename) == 3) {
                // If successful, call the function to list the specified number of student grades from the specified page.
                listSome(filename, numEntries, pageNumber);
            } else {
                // Display an error message if the command usage is incorrect.
                fprintf(stderr, "Error: Incorrect usage of listSome.\n");
            }
        }
        // Handle any unrecognized commands.
        else {
            // Inform the user that the entered command is not recognized.
            printf("Invalid command.\n");
        }
	
    }
}
/*---------------------------------------------------*/

// Function to create a file named `fileName`. If the file already exists, it won't be overwritten.
void createFile(const char* fileName) {
    pid_t pid = fork(); // Create a child process.
    char logBuffer[256]; // Buffer for constructing log messages.

    if (pid == -1) {
        // If fork() fails, log and display an error.
        perror("fork");
        snprintf(logBuffer, sizeof(logBuffer), "Failed to fork process for creating file: %s", fileName);
        logMessage(logBuffer); // Use the logMessage function to log this failure.
    } else if (pid == 0) {
        // Child process: attempt to create the file.
        int fd = open(fileName, O_WRONLY | O_CREAT | O_EXCL, 0644); // Attempt to open (and possibly create) the file.
        if (fd == -1) {
            // If opening the file fails, log and display an error, then exit with failure.
            perror("open");
            snprintf(logBuffer, sizeof(logBuffer), "Failed to create file: %s", fileName);
            logMessage(logBuffer); // Log the failure to create the file.
            exit(EXIT_FAILURE); // Terminate the child process with an error status.
        }
        // If successful, print and log a success message.
        printf("File '%s' created successfully.\n", fileName);
        snprintf(logBuffer, sizeof(logBuffer), "File '%s' created successfully.", fileName);
        logMessage(logBuffer); // Log the successful file creation.
        close(fd); // Close the file descriptor.
        exit(EXIT_SUCCESS); // Terminate the child process successfully.
    } else {
        // Parent process: wait for the child process to complete.
        wait(NULL); // Wait for the child process to terminate.
        // Log that the parent has completed waiting for the child.
        snprintf(logBuffer, sizeof(logBuffer), "Completed file creation process for: %s", fileName);
        logMessage(logBuffer); // Log the completion of the file creation process.
    }
}

/*---------------------------------------------------*/
// Function to add a student's grade to a specified file.
void addStudentGrade(const char* filename, const char* name, const char* grade) {
    pid_t pid = fork(); // Create a child process.
    char logBuffer[256]; // Buffer to store log messages.

    if (pid == -1) {
        // If forking fails, log the error and exit with failure.
        perror("fork");
        snprintf(logBuffer, sizeof(logBuffer), "Failed to fork process for adding student grade: %s, %s", name, grade);
        logMessage(logBuffer); // Log the fork failure.
        exit(EXIT_FAILURE); // Exit the current process with failure status.
    } else if (pid == 0) {
        // Child process: attempt to open (or create if not existing) the file in append mode.
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            // If opening the file fails, log the error and exit with failure.
            perror("open");
            snprintf(logBuffer, sizeof(logBuffer), "Failed to open or create file: %s", filename);
            logMessage(logBuffer); // Log the open failure.
            exit(EXIT_FAILURE); // Exit the child process with failure status.
        }

        // Prepare the entry to be written to the file.
        char buffer[256];
        int length = snprintf(buffer, sizeof(buffer), "%s, %s\n", name, grade);
        if (length > 0) {
            // Attempt to write the entry to the file.
            if (write(fd, buffer, length) != length) {
                // If writing fails, log the error and exit with failure.
                perror("write");
                snprintf(logBuffer, sizeof(logBuffer), "Failed to write student grade to file: %s", filename);
                logMessage(logBuffer); // Log the write failure.
                close(fd); // Close the file descriptor.
                exit(EXIT_FAILURE); // Exit the child process with failure status.
            }
        }

        // Log the success message and exit the child process with success status.
        snprintf(logBuffer, sizeof(logBuffer), "Student grade added successfully for: %s, %s", name, grade);
        logMessage(logBuffer); // Log the successful addition.
        close(fd); // Close the file descriptor.
        exit(EXIT_SUCCESS); // Exit the child process with success status.
    } else {
        // Parent process: wait for the child process to complete and check its exit status.
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to terminate.
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            // If the child exited successfully, print and log the success message.
            printf("Student grade added successfully.\n");
            snprintf(logBuffer, sizeof(logBuffer), "Successfully added student grade for: %s, %s", name, grade);
            logMessage(logBuffer); // Log the successful addition in the parent process.
        } else {
            // If an error occurred in the child process, print and log the error message.
            printf("Error adding student grade.\n");
            snprintf(logBuffer, sizeof(logBuffer), "Error encountered while adding student grade for: %s, %s", name, grade);
            logMessage(logBuffer); // Log the error encountered in the child process.
        }
    }
}

/*---------------------------------------------------*/

// Function to search for a student by name within a file.
void searchStudent(const char* fileName, const char* name) {
    char logBuffer[512]; // Buffer for constructing log messages.
    pid_t pid = fork(); // Create a child process to handle the search operation.

    // Log the beginning of the search operation.
    snprintf(logBuffer, sizeof(logBuffer), "Starting search for student '%s' in file: %s", name, fileName);
    logMessage(logBuffer);

    if (pid == -1) {
        // If forking fails, log the error and exit.
        perror("fork");
        snprintf(logBuffer, sizeof(logBuffer), "Failed to fork process for searching student: %s", name);
        logMessage(logBuffer);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: attempt to open the specified file in read-only mode.
        int fd = open(fileName, O_RDONLY);
        if (fd == -1) {
            // If opening the file fails, log the error and exit.
            perror("open");
            snprintf(logBuffer, sizeof(logBuffer), "Failed to open file: %s for searching", fileName);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        }

        // Read the file content and search for the student name.
        const size_t buffer_size = 1024;
        char buffer[buffer_size];
        ssize_t bytes_read;
        char* line = NULL;
        size_t line_len = 0;
        int found = 0; // Flag to indicate if the student was found.

        // Loop through the file content.
        while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
            for (ssize_t i = 0; i < bytes_read; ++i) {
                // Dynamically allocate or resize the line buffer.
                char* new_line = realloc(line, line_len + 1);
                if (!new_line) {
                    // If memory allocation fails, log the error, clean up, and exit.
                    perror("realloc");
                    free(line);
                    snprintf(logBuffer, sizeof(logBuffer), "Memory allocation error during search");
                    logMessage(logBuffer);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
                line = new_line;
                line[line_len] = buffer[i]; // Append character to the line buffer.

                // Check if the end of the line or file has been reached.
                if (buffer[i] == '\n' || i == bytes_read - 1) {
                    line[line_len] = '\0'; // Null-terminate the current line.
                    // If the line contains the student's name, print and set found flag.
                    if (strstr(line, name) != NULL) {
                        printf("%s\n", line); // Print the matching student record.
                        found = 1;
                        break; // Break the loop as the student has been found.
                    }
                    // Reset the line buffer for the next line.
                    free(line);
                    line = NULL;
                    line_len = 0;
                } else {
                    line_len++; // Increment line length for the next character.
                }
            }
            if (found) break; // Exit the outer loop if the student has been found.
        }

        // Clean up.
        free(line);
        close(fd);

        // Handle read errors or log if the student was not found or found successfully.
        if (bytes_read == -1) {
            // Log any errors encountered while reading the file.
            perror("read");
            snprintf(logBuffer, sizeof(logBuffer), "Error occurred while reading file: %s during search", fileName);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        } else if (!found) {
            // Log if the student name was not found in the file.
            printf("Student not found.\n");
            snprintf(logBuffer, sizeof(logBuffer), "Student '%s' not found in file: %s", name, fileName);
            logMessage(logBuffer);
        } else {
            // Log a successful search operation.
            snprintf(logBuffer, sizeof(logBuffer), "Successfully found student '%s' in file: %s", name, fileName);
            logMessage(logBuffer);
        }

        exit(EXIT_SUCCESS); // Exit the child process successfully.
    } else {
        // Parent process: wait for the child process to complete.
        int status;
        waitpid(pid, &status, 0); // Wait for the child to finish.
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE) {
            // Notify if an error occurred during the search in the child process
            printf("An error occurred during search.\n");
        }
    }
}

/*---------------------------------------------------*/

// Function to compare two Student structures for sorting.
// Parameters 'a' and 'b' are pointers to the elements to be compared, which are cast to the Student type within the function.
int compareStudents(const void* a, const void* b) {
    // Cast void pointers to Student pointers for easy access to Student fields.
    const Student* studentA = (const Student*)a;
    const Student* studentB = (const Student*)b;
    int result = 0; // Initialize result of comparison to 0 (indicates equality).

    // Decide on the sorting criteria based on the global variable `currentSortCriteria`.
    switch (currentSortCriteria) {
        case SORT_BY_NAME:
            // Compare students by name using strcmp, which returns <0, 0, or >0
            // if studentA's name is lexicographically less than, equal to, or greater than studentB's name, respectively.
            result = strcmp(studentA->name, studentB->name);
            break;
        case SORT_BY_GRADE:
            // Compare students by grade in a similar manner.
            result = strcmp(studentA->grade, studentB->grade);
            break;
    }

    // If the global sorting order is DESCENDING, reverse the comparison result.
    // This is because strcmp's return value assumes an ascending order by default.
    if (currentSortOrder == DESCENDING) {
        result = -result;
    }

    return result; // Return the result of comparison.
}

// Function to sort student records in a file based on the current sorting criteria and order.
void performSorting(const char* fileName) {
    char logBuffer[512]; // Buffer to hold log messages.
    pid_t pid = fork(); // Fork a new process to handle sorting.

    // Log the start of the sorting process.
    snprintf(logBuffer, sizeof(logBuffer), "Starting sorting process for file: %s", fileName);
    logMessage(logBuffer);

    if (pid == -1) {
        // If the fork fails, log the error and exit.
        perror("fork");
        snprintf(logBuffer, sizeof(logBuffer), "Failed to fork process for sorting file: %s", fileName);
        logMessage(logBuffer);
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process begins here.
        // Attempt to open the specified file in read-only mode.
        int fd = open(fileName, O_RDONLY);
        if (fd == -1) {
            // If the file can't be opened, log the error and exit.
            perror("open");
            snprintf(logBuffer, sizeof(logBuffer), "Failed to open file for sorting: %s", fileName);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        }

        Student students[100]; // Array to hold student records. Adjust size as necessary.
        int count = 0; // Counter for the number of student records read.
        char buffer[1024]; // Buffer for reading file content.
        ssize_t bytes_read; // Number of bytes read from the file.

        // Read and process the file content.
        while ((bytes_read = read(fd, buffer, sizeof(buffer)-1)) > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the buffer.

            // Extract student records from the buffer and populate the students array.
            char* line = strtok(buffer, "\n");
            while (line != NULL && count < 100) {
                char* commaPos = strchr(line, ',');
                if (commaPos) {
                    *commaPos = '\0'; // Split the line at the comma.
                    strncpy(students[count].name, line, sizeof(students[count].name) - 1);
                    strncpy(students[count].grade, commaPos + 2, sizeof(students[count].grade) - 1);
                    count++;
                }
                line = strtok(NULL, "\n");
            }
        }
        close(fd); // Close the file after reading.

        // Check for read errors.
        if (bytes_read == -1) {
            perror("read");
            snprintf(logBuffer, sizeof(logBuffer), "Error reading from file during sorting: %s", fileName);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        }

        // Sort the student records using the qsort function and the compareStudents comparator.
        qsort(students, count, sizeof(Student), compareStudents);

        // Log the sorted students (optional)
        for (int i = 0; i < count; i++) {
            printf("%s, %s\n", students[i].name, students[i].grade);
        }

        // Optionally log the outcome of the sorting operation.
        snprintf(logBuffer, sizeof(logBuffer), "Sorting completed successfully for file: %s", fileName);
        logMessage(logBuffer);
        exit(EXIT_SUCCESS); // Exit the child process successfully.
    } else { // Parent process waits for the child to complete.
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish.
        // Check if the child process exited with an error.
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            snprintf(logBuffer, sizeof(logBuffer), "An error occurred in child process while sorting file: %s", fileName);
            logMessage(logBuffer);
            fprintf(stderr, "An error occurred while sorting.\n");
        }
    }
}

// Function to set sorting criteria and order for student records, then sort them.
void sortAll(const char* fileName, SortCriteria criteria, SortOrder order) {
    
    // Update global variables to the specified sorting criteria and order.
    // This approach allows the sorting behavior to be dynamically adjusted
    // based on user input or other conditions within the program.
    currentSortCriteria = criteria; // Set the global sorting criteria (e.g., by name or grade).
    currentSortOrder = order;       // Set the global sorting order (ascending or descending).
    
    // After setting the criteria and order, perform the sorting operation.
    // This function will use the global variables (currentSortCriteria and currentSortOrder)
    // to determine how the sorting should be conducted.
    performSorting(fileName);
}

/*---------------------------------------------------*/

// Function to display help information about the available commands in the student grade management system.
void displayHelp() {
    // General introduction to the available commands.
    printf("Available commands:\n");
    
    // Explains how to display the help message itself.
    printf("  gtuStudentGrades - Displays this help message - ex: gtuStudentGrades\n");
    
    // Describes how to create a new file for storing student grades.
    printf("  gtuStudentGrades filename - Creates a new file - ex: gtuStudentGrades grades.txt\n");
    
    // Provides the syntax for adding a new student grade to the specified file.
    printf("  addStudentGrade filename \"Name Surname\" \"Grade\" - Adds a new student grade to the file - ex: addStudentGrades grades.txt \"cagri\" \"AA\" \n");
    
    // Outlines how to search for a student's grade by their name.
    printf("  searchStudent filename \"Name Surname\" - Searches for a student's grade by name - ex: searchStudent grades.txt \"cagri\" \n");
    
    // Details the command to sort student names in ascending order by default.
    printf("  sortAll filename - Sorts student names in ascended order by default - ex: sortAll grades.txt\n");
    
    // Explains how to sort students and grades by specified criteria and order.
    printf("  sortAll filename [name|grade] [asc|desc] - Sorts student & grades by specified criteria and order - ex: sortAll grades.txt name desc \n");
    
    // Describes the command to print all entries in the file.
    printf("  showAll filename - Prints all entries in the file - ex: showAll grades.txt\n");
    
    // Explains how to print the first 5 entries in the file.
    printf("  listGrades filename - Prints first 5 entries in the file - ex: listGrades grades.txt\n");
    
    // Details the command to list students according to the given information (number of entries, page number).
    printf("  listSome numOfEntries pageNumber filename - Lists students according to given info - ex: listSome 5 2 grades.txt \n");
    
    // You can add other commands and their descriptions as necessary.
}

/*---------------------------------------------------*/

// Function to display all student grade entries from the specified file.
void showAll(const char* filename) {
    processFileWithFork(filename, 0, -1); // Call processFileWithFork to show all lines.
    // The arguments 0 and -1 indicate starting from the first line and showing all lines, respectively.
}

/*---------------------------------------------------*/

// Function to display the first 5 student grade entries from the specified file.
void listGrades(const char* filename) {
    processFileWithFork(filename, 0, 5); // Call processFileWithFork to show the first 5 entries.
    // The arguments 0 and 5 indicate starting from the first line and showing the first 5 lines, respectively.
}

/*---------------------------------------------------*/

// Function to display a specific "page" of student grade entries from the specified file.
void listSome(const char* filename, int numEntries, int pageNumber) {
    int startLine = (pageNumber - 1) * numEntries; // Calculate the starting line based on the page number and number of entries per page.
    processFileWithFork(filename, startLine, numEntries); // Call processFileWithFork to show a specified range of lines.
    // This effectively paginates the entries, showing a set number of entries from a specific starting point.
}

/*---------------------------------------------------*/

// Function to process specific lines from a file using a separate process.
void processFileWithFork(const char* filename, int startLine, int numLines) {
    char logBuffer[512]; // Buffer for log messages

    pid_t pid = fork(); // Create a new process

    if (pid == -1) {
        // Fork failed, log the error, and exit.
        perror("fork");
        snprintf(logBuffer, sizeof(logBuffer), "Failed to fork process for processing file: %s", filename);
        logMessage(logBuffer);
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process starts here.
        // Attempt to open the file in read-only mode.
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            // Opening the file failed, log the error, and exit.
            perror("open");
            snprintf(logBuffer, sizeof(logBuffer), "Failed to open file: %s", filename);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        }

        char buffer[MAX_LINE_LENGTH];
        ssize_t bytes_read;
        int currentLine = 0, linesPrinted = 0; // Trackers for current line and number of lines printed.

        // Log the start of file processing.
        snprintf(logBuffer, sizeof(logBuffer), "Starting to process file: %s", filename);
        logMessage(logBuffer);
        
        // Read the file content.
        while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
            for (ssize_t i = 0; i < bytes_read; ++i) {

                // Check if the current line is within the desired range to print.
                if ((currentLine >= startLine && linesPrinted < numLines) || numLines == -1) {
                    // Write the current character to stdout.
                    write(STDOUT_FILENO, &buffer[i], 1);
                    // Increment the printed lines counter after printing a newline, if within range.
                    if (buffer[i] == '\n' && currentLine >= startLine) {
                        linesPrinted++;
                        if (linesPrinted == numLines) break; // Break if the desired number of lines have been printed.
                    }
                }

                // Increment line counter on newline characters.
                if (buffer[i] == '\n') {
                    currentLine++;
                }
            }
            if (linesPrinted == numLines) break; // Break the outer loop if the desired number of lines have been printed.
        }

        // Close the file descriptor.
        close(fd);

        // Handle potential read errors.
        if (bytes_read == -1) {
            perror("read");
            snprintf(logBuffer, sizeof(logBuffer), "Error occurred while reading file: %s", filename);
            logMessage(logBuffer);
            exit(EXIT_FAILURE);
        }

        // Log successful file processing.
        snprintf(logBuffer, sizeof(logBuffer), "Successfully processed file: %s", filename);
        logMessage(logBuffer);
        exit(EXIT_SUCCESS); // Signal successful exit from the child process.
    } else { // Parent process.
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish.
        // Check for errors in the child process's execution.
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            // Log and print error message if the child process encountered an issue.
            snprintf(logBuffer, sizeof(logBuffer), "An error occurred while processing the file: %s in child process.", filename);
            logMessage(logBuffer);
            fprintf(stderr, "An error occurred while processing the file.\n");
        }
    }
}

/*---------------------------------------------------*/

// Function to log a message with a timestamp to "application.log".
void logMessage(const char* message) {
    pid_t pid = fork(); // Create a new process to handle the logging operation.

    if (pid == -1) {
        // If the fork fails, report the error and exit from the current process.
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: responsible for writing the log message.
        FILE* logFile = fopen("application.log", "a"); // Attempt to open the log file in append mode.
        if (logFile == NULL) {
            // If opening the log file fails, report the error and exit from the child process.
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }

        // Obtain the current time.
        time_t now = time(NULL);
        char* timeString = ctime(&now); // Convert the time to a string.
        timeString[strlen(timeString) - 1] = '\0'; // Remove the newline character at the end of the time string.

        // Write the log message to the file with a timestamp.
        fprintf(logFile, "[%s] %s\n", timeString, message);

        fclose(logFile); // Close the log file to ensure the message is written.
        exit(EXIT_SUCCESS); // Exit successfully from the child process.
    } else {
        // Parent process: wait for the child process to finish logging.
        wait(NULL);
    }
}

/*---------------------------------------------------*/
