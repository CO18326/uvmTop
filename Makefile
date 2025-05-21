# Makefile for building uvmTop from uvmTop3.c

CC = gcc
CFLAGS = -g -pthread
TARGET = uvmTop
SRC = uvmTop3.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS)

clean:
	rm -f $(TARGET)

