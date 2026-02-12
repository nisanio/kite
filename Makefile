CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

SRC = src/main.c \
      src/value.c \
      src/env.c \
      src/ast.c \
      src/lexer.c

OBJ = $(SRC:.c=.o)

TARGET = kite

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)
