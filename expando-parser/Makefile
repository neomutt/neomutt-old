CC := gcc
CFLAGS := -std=c99 -Wall -Wextra

.PHONY: all run clean

all: parser

parser: parser.c
	$(CC) $(CFLAGS) -o $@ $<

run: parser
	./parser

clean:
	rm -rf parser

