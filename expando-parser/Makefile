CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -O0 -ggdb

# CFLAGS	+= -fsanitize=address -fno-omit-frame-pointer -fno-common

.PHONY: all run clean test update-test

all: parser

parser.o: parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_parser.o: test_parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

parser: test_parser.o parser.o
	$(CC) $(CFLAGS) -o parser $^

run: parser
	./parser "%<c?%a&%b> %d";

dbg: parser
	gdb -ex run ./parser "%<c?%a&%b> %d";

clean:
	rm -rf parser
	rm -rf *.o

test: parser
	./run_tests.sh
