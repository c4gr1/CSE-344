# Makefile

# Compiler
CC = gcc

# Sources
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Targets
SERVER_TARGET = server
CLIENT_TARGET = client

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Build server
$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) -o $(SERVER_TARGET) $(SERVER_SRC) -lpthread -lrt

# Build client
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

# Clean up generated files
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all clean

