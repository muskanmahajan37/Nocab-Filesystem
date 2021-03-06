#ifndef PAGES_H
#define PAGES_H

#include <stdio.h>

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes for file
    int xtra; // more stuff can go here	
	      // symlinks store next pnum here
} pnode;

void   pages_init(const char* path);
void   pages_free();
void*  pages_get_page(int pnum);
pnode* pages_get_node(int pnum);
int    pages_find_empty();
void   print_node(pnode* node);

#endif
