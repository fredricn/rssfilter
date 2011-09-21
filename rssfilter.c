/*	rssfilter.c
 *
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


#ifdef LIBXML_TREE_ENABLED

#define RSS_TAG_START "<rss>"
#define RSS_TAG_END "</rss>"

int pattern_check(char *content) {
	
	regex_t regex;
	int r;
	
	r = regcomp(&regex, "^Recension", 0);
	if (r) {
		printf("Could not compile regexp. Pattern invalid?\n");
		return 0;
	}
	
	return (regexec(&regex, content, 0, NULL, 0) == 0) ? 1 : 0;
}


void remove_node(xmlNodePtr *node) {
	
	xmlNodePtr tmp_node;
	
	tmp_node = (*node)->prev;
	xmlUnlinkNode((*node));
	xmlFreeNode((*node));
	*node = tmp_node;
}

char *filters(char *str) {
	
	xmlDocPtr doc;
	xmlNodePtr node, i_node, t_node, tmp_node;
	xmlChar *xmlbuff, *n_content;
	int buffersize;
	
	doc = xmlReadMemory(str, strlen(str), "noname.xml", NULL, 0);
	
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse document\n");
		return;
	}
	
	/* rss node */
	node = xmlDocGetRootElement(doc);
	
	/* get channel node */
	node = xmlFirstElementChild(node);
	if (node == 0) {
		printf("Could not find channel node");
	}
	
	
	/* loop through items */
	for (i_node = node->children; i_node; i_node = i_node->next) {
		if (i_node->type == XML_ELEMENT_NODE
				&& strcmp(i_node->name,"item") == 0) {
			
			t_node = xmlFirstElementChild(i_node);
			n_content = xmlNodeGetContent(t_node);
				
			if (!pattern_check((char *)n_content)) {				
				
				/* remove item node*/
				remove_node(&i_node);
				
				/* remove "space"(text) node*/
				remove_node(&i_node);
			}
		} 
	}
	
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize,1);
	
	xmlFreeDoc(doc);
	
	return xmlbuff;
}


char *get_stdin(char *str) {
	
	char *r, *pch;
	char buff[1024];
	int c = 1;
	
	 do {
		
		if (c == 1) {
			str = malloc(sizeof(char)*1024);
		} else {
			str = realloc(str,sizeof(char)*(1024*c));
		}

		fread(buff, 1024, 1, stdin);
		strncat(str,buff, 1024);
		
		++c;
	 } while(!feof(stdin));
	 
	 /* find feed-end hack */
	 pch = strstr(str,RSS_TAG_END);
	 pch = pch + 6;
	 *pch = '\0';

	 return str;
}

int is_ready(int fd) {
    fd_set fdset;
    struct timeval timeout;
    int ret;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    timeout.tv_sec = 3;
    timeout.tv_usec = 1;
    
    return select(1, &fdset, NULL, NULL, &timeout) == 1 ? 1 : 0;
}


int main(const int argc, char **argv) {
	
	char *str;
	
	/* get feed from stdin */
	if (is_ready(fileno(stdin))) {
		str = get_stdin(str);
	} else {
		fprintf(stderr, "no feed\n");
		return EXIT_FAILURE;
	}
	
	/* run filters */
	str = filters(str);
	
	/* output filtered xml */
	printf("%s", str);
	
	free(str);
	
	return EXIT_SUCCESS;
}

#else

int main() {
    fprintf(stderr, "xmllib tree support not compiled in\n");
    return EXIT_FAILURE;
}

#endif
