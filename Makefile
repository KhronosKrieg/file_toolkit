# Makefile for File Toolkit

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = out

# Source files
SRC = file_toolkit.c
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up
clean:
	rm -f $(OBJ) $(TARGET)

# Run with help
run:
	./$(TARGET) --help

.PHONY: all clean run
