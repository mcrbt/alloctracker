NAME = alloctracker
VERSION = 0.5.3
CC = gcc
CSTD = -std=gnu99
ARCH = 64
WARN = -Wall -Werror -Wextra -Wstrict-prototypes -Wunused \
	-pedantic -pedantic-errors
CFLAGS = -c -ggdb -m$(ARCH) $(CSTD) $(WARN) \
	-DALLOC_TRACKER_VERSION='"$(VERSION)"' -DAT_TRUNCATE_BACK=0

.PHONY: all test clean pack

all: obj/$(NAME).o

test: obj/$(NAME).o obj/at_test.o
	mkdir -p bin
	$(CC) -o bin/$(NAME)_test $^

obj/$(NAME).o: src/$(NAME).c \
	src/$(NAME)_intern.h
	mkdir -p obj
	$(CC) $(CFLAGS) -o $@ $<

obj/at_test.o: src/at_test.c src/$(NAME).h
	$(CC) $(CFLAGS) -DAT_ALLOC_TRACK -o $@ $<

clean:
	rm -f obj/$(NAME).o obj/at_test.o bin/$(NAME)_test
	rm -f src/*~ core.* *~

pack:
	tar cJf $(NAME)_$(VERSION).txz src/ rsc/ Makefile LICENSE README* .gitignore
