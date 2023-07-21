CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -O0 -ggdb

.PHONY: all run clean

all: parser

parser: parser.c
	$(CC) $(CFLAGS) -o $@ $<

run: parser
	./parser "%<c?%a&%b> %d";

dbg: parser
	gdb -ex run ./parser "%<c?%a&%b> %d";

clean:
	rm -rf parser

test: parser
	./parser "test text";
	./parser "test text%% %a %4b";
	./parser "%X %8X %-8X %08X %.8X %8.8X %-8.8X";
	./parser "test text%% %aa %4ab %bb";
	./parser " %[%b %d]  %{!%b %d} %(%b %d)";
	./parser "%|A %>B %*C";
	./parser "if: %?l?%4l?  if-else: %?l?%4l&%4c?";
	./parser "if: %<l?%4l>  if-else: %<l?%4l&%4c>";
	./parser "nested: %<c?%<a?%c&%d>&%<b?%e&%f>>";
	./parser "%@hook1@ %a %@hook2@";
	./parser "%4C %Z %[%b %d %H:%M] %-15.15L (%<l?%4l&%4c>) %s";
	./parser "%<c?%a&%b> %d";

