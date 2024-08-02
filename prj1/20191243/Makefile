CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = debug.c hex_dump.c list.c hash.c bitmap.c mapping.c main.c
OBJS = $(SRCS:.c=.o)
TARGET = testlib

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
