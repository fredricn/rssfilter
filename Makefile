
CC = gcc
CFLAGS = -g -Wall -O2 -I/usr/include/libxml2
LDFLAGS = -lxml2
LIBXML=`xml2-config --cflags --libs`

rssfilter: rssfilter.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

all: rssfilter

clean :
	rm *.o
