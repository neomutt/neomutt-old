CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -O0 -ggdb

.PHONY: all run clean

all: parser

parser: parser.c
	$(CC) $(CFLAGS) -o $@ $<

run: parser
	./parser

dbg: parser
	gdb -ex run ./parser

clean:
	rm -rf parser

