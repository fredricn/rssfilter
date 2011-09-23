
CC = gcc
CFLAGS = -g -Wall -O2 -I/usr/include/libxml2
LDFLAGS = -lxml2
LIBXML=`xml2-config --cflags --libs`

rssfilter: obj
	$(CC) $(CFLAGS) $(LDFLAGS) *.o -o rssfilter
	
obj:
	$(CC) $(CFLAGS) $(LDFLAGS) -c *.c

all: rssfilter

clean :
	rm *.o