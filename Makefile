CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = test_instinct

all: $(TARGET)

$(TARGET): instinct.c test_instinct.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
