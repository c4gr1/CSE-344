# Makefile for C project

CC=gcc
CFLAGS=-Wall -g
TARGET=program
SOURCES=test.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
	rm -f /tmp/fifo1 /tmp/fifo2

run:
	./$(TARGET) $(ARGS)

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) $(ARGS)

