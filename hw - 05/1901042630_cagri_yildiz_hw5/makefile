# Makefile

# Compiler and flags
CC = gcc
CFLAGS = -pthread -Wall

# Executable name
TARGET = MWCp

# Source files
SRCS = 1901042630_main.c

# Default target
all: $(TARGET)

# Compile the program
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Run the program with buffer size 1000 and 2 workers
run: $(TARGET)
	./$(TARGET) 10 10 ../testdir ../tocopy

# Run Valgrind to check for memory leaks
valgrind: $(TARGET)
	valgrind --leak-check=full --track-origins=yes ./$(TARGET) 10 10 ../testdir/src/libvterm ../tocopy

# Clean the build
clean:
	rm -f $(TARGET)

