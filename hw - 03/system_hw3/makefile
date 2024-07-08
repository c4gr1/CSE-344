# Makefile

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread -Wall -Wextra -Werror

# Target executable name
TARGET = parking

# Source files
SRCS = parking.c

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run Valgrind to detect memory leaks
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Clean the build
clean:
	rm -f $(TARGET)

