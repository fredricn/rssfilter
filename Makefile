

LIBXML=`xml2-config --cflags --libs`

all:
	gcc $(LIBXML) -o rssfilter rssfilter.c
clean :
	rm rssfilter