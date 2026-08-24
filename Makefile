CC = gcc
CFLAGS = -O2 -Wall

CFLAGS += $(shell pkg-config --cflags hunspell)
LDLIBS += $(shell pkg-config --libs hunspell)

huncolor: huncolor.o
huncolor.o: huncolor.c

install: huncolor
	install huncolor $(HOME)/bin

clear: huncolor
	rm -rf huncolor *.o
