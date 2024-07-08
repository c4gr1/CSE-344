CC = gcc
CFLAGS = -Wall -pthread -g
LDFLAGS = -lm
DEPS = common.h protocol.h utils.h
OBJ_SERVER = server.o cook.o delivery.o manager.o utils.o
OBJ_CLIENT = client.o utils.o

# .o files from .c files
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Target for both server and client
all: server client

# Server executable
server: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Client executable
client: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f *.o server client pide_shop_log.txt

# Run client with specified arguments
run_client: client
	./client 127.0.0.1 600 6 8

# Run server with specified arguments
run_server: server
	./server 127.0.0.1 6 6 1000

.PHONY: all clean run_client run_server
