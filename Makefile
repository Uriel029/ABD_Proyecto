CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -std=c11
SRC = src/main.c src/database.c src/parser.c
OBJ = $(SRC:.c=.o)
TARGET = db_engine

.PHONY: all setup clean clean-all run

all: setup $(TARGET)

setup:
	mkdir -p data

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/main.o: src/main.c include/database.h include/parser.h
src/database.o: src/database.c include/database.h
src/parser.o: src/parser.c include/parser.h include/database.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

clean-all: clean
	rm -rf data/*

run: $(TARGET)
	./$(TARGET)
