# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -std=c11 -pedantic -pthread -Wno-format-truncation

# Libraries
LIBS = -lcurl

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = crawler

# Default target
all: $(EXEC)

# Rule to build the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) $(LIBS)

# Rule to compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(EXEC) $(OBJ) crawler_log.txt page*.html urls.txt

# Run rule
run: $(EXEC)
	./$(EXEC)