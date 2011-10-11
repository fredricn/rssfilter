/*	
 *  Copyright (C) 2011  Fredric Nilsson <fredric@fiktivkod.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <regex.h>

#include <libxml/parser.h>
#include <libxml/tree.h>


#define APPLICATION "rssfilter"

#define RSS_TAG_START "<rss>"
#define RSS_TAG_END "</rss>"
#define RSS_TAG_CHAN "<channel>";


typedef struct {
	
	char *pattern;
	struct Regexfilter *next;
	struct Regexfilter *prev;
	
} Regexfilter;



void usage() 
{
	printf("%s [flags]\n"
		   "\t-t seconds\tTime to wait for input from stdin\n"
		   "\t-e pattern\tRegex pattern to use for filtering\n", APPLICATION);
	
}


Regexfilter *filter_new(Regexfilter *prev, char *pattern)
{
	
	Regexfilter *node = (Regexfilter*)calloc(1,sizeof(Regexfilter));
	
	if (prev != NULL) {
		prev->next = node;
		node->prev = prev;
	}
	
	node->pattern = malloc(sizeof(char)*strlen(pattern)+1);
	strcpy(node->pattern,pattern);
	
	return node;
}

void filter_set_pattern(Regexfilter *node, char *pattern)
{
	
	free(node->pattern);
	node->pattern = malloc(sizeof(char)*strlen(pattern)+1);
	strcpy(node->pattern,pattern);
}

/*
 * Remove current node and returns a pointer 
 * to the previous node.
 */
Regexfilter *filter_remove(Regexfilter *node) 
{
	Regexfilter *new_node;
	
	/* the old node is gone */
	new_node = node->prev;
	new_node->next = node->next;
	free(node);
	
	/* still need to correct "->next->prev" */
	node = new_node->next;
	node->prev = new_node;
	
	return new_node;
}

void filter_destroy(Regexfilter *node) 
{
	
	Regexfilter *a = node,
				*b;
	
	/* get last node */
	while (a->next != NULL) {
		a = a->next;
	}
	
	/* start destroying */
	b = a;
	while (b->prev != NULL) {
		a = b;
		b = b->prev;
		free(a->pattern);
		free(a);
	}

	free(b->pattern);
	free(b);
}



int pattern_check(char *content, Regexfilter *node) 
{
	regex_t regex;
	int r;
	
	if (node != NULL) {
		
		do {
			
			if (node->pattern != NULL) {
				r = regcomp(&regex, node->pattern, 0);
				if (r) {
					printf("Could not compile regexp. Pattern invalid?\n");
					return 0;
				}

				if (regexec(&regex, content, 0 , NULL, 0) == 0) {
					return 1;
				}
			}

		} while ((node = node->next) != NULL);
	}
	
	return 0;
}


void feed_remove_node(xmlNodePtr *node) 
{
	xmlNodePtr tmp_node;
	
	tmp_node = (*node)->prev;
	xmlUnlinkNode((*node));
	xmlFreeNode((*node));
	*node = tmp_node;
}

xmlNodePtr feed_first_item(xmlDocPtr *doc)
{
	xmlNodePtr node, i_node;
	
	/* get <rss> node */
	node = xmlDocGetRootElement(doc);
	if (node == 0) {
		fprintf(stderr, "Could not find rss node");
		return EXIT_FAILURE;
	}
	
	/* get <channel> node */
	node = xmlFirstElementChild(node);
	if (node == 0) {
		fprintf(stderr, "Could not find channel node");
		return EXIT_FAILURE;
	}
	
	/* find first <item> node */
	for (i_node = node->children; i_node; i_node = i_node->next) {
		if (strcmp(i_node->name,"item") == 0)
			return i_node;
	}
	
	return NULL;
}


char *filter(char *str, Regexfilter *filterList) 
{
	
	xmlDocPtr doc;
	xmlNodePtr item_node, title_node;
	xmlChar *xmlbuff, *title_content;
	int buffersize;
	
	doc = xmlReadMemory(str, strlen(str), "noname.xml", NULL, 0);
	
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse document\n");
		return;
	}
	
	item_node = feed_first_item(doc);
	
	/* loop through items */
	while (item_node != NULL) {
		
		title_node = xmlFirstElementChild(item_node); 
		title_content = xmlNodeGetContent(title_node);

		if (!pattern_check((char *)title_content, filterList)) {
			/* remove <item> node and leftover space*/
			feed_remove_node(&item_node);
			feed_remove_node(&item_node);
		}
		
		item_node = xmlNextElementSibling(item_node);
	}

	
	/* 
	 * Convert rss doc to string 
	 * and return.
	 */
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize,1);
	
	xmlFreeDoc(doc);
	
	return xmlbuff;
}


char *stdin_get(char *str) 
{
	
	char *pch;
	char buff[512];
	int c = 1;
	
	do {
		
		if (c == 1) {
			str = malloc(sizeof(char)*512);
		} else {
			str = realloc(str,sizeof(char)*(512*c));
		}
		
		fread(buff, 512, 1, stdin);
		strncat(str,buff, 512);
		
		++c;
	} while(!feof(stdin));
	
	/* feed-end hack */
	pch = strstr(str,RSS_TAG_END) + 6;
	*pch = '\0';
	
	return str;
}

int stdin_ready(int fd, int timeout) 
{
    fd_set fdset;
    struct timeval tout;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    tout.tv_sec = timeout;
    tout.tv_usec = 1;
    
    return select(1, &fdset, NULL, NULL, &tout) == 1 ? 1 : 0;
}


int main(const int argc, char **argv)
{
	
	char *feed;
	Regexfilter *filterList = filter_new(NULL,".*");
	int i, j,
		timeout = 3;

	/* supply regex filter */
	if (argc > 1) {
		for (i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-e") == 0) {
				filter_set_pattern(filterList, argv[i+1]);
				
				for (j = i+2; j < argc ; j++) {

					if (argv[j][0] == '-')
						break;
					
					filter_new(filterList, argv[j]);
				}
				
			}
			if (strcmp(argv[i], "-t") == 0) {
				timeout = atoi(argv[i+1]);
			}
			if (strcmp(argv[i], "-h") == 0) {
				usage();
				return EXIT_SUCCESS;
			}
		}
	} 

	/* get feed from stdin */
	if (stdin_ready(fileno(stdin), timeout)) {
		feed = stdin_get(feed);
	} else {
		fprintf(stderr, "no feed\n");
		return EXIT_FAILURE;
	}
	
	
	/* run filters */
	feed = filter(feed, filterList);
	
	
	/* output filtered xml */
	printf("%s", feed);
	

	free(feed);
	filter_destroy(filterList);
	
	return EXIT_SUCCESS;
}
