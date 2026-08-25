CC = gcc

CFLAGS = -O2 -Wall
CFLAGS += $(shell pkg-config --cflags hunspell)

LDLIBS += $(shell pkg-config --libs hunspell)


OBJS = huncolor.o parser.o

huncolor: $(OBJS)
	$(CC) $(OBJS) $(LDLIBS) -o huncolor

huncolor.o: huncolor.c 
	$(CC) $(CFLAGS) -c huncolor.c

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

install: huncolor
	install -m 755 huncolor $(HOME)/bin/huncolor

clean:
	rm -f huncolor *.o

.PHONY: install clean