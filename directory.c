#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "directory.h"
#include "pages.h"
#include "slist.h"


void directory_init() {
    // root node
    pnode* rn = pages_get_node(1);
    if (rn->refs == 0) {
        rn->size = 4096;
        rn->refs = 1;
        rn->mode = 040755;//mode indicates directory
        rn->xtra = 0;
    }
}

directory directory_from_pnum(int pnum) {
    // iterate over array of directories until find one with pnum
    void* page = pages_get_page(pnum);
    dirent* dirents = (dirent*) page;
    directory rv = *((directory*) malloc(sizeof(directory)));
    rv.pnum = pnum;
    rv.ents = dirents;
    rv.node = pages_get_node(pnum);
	// TODO: double check error handeling here.
    //if (S_ISDIR(rv.node->mode)) {
        return rv;
    //}
    //return NULL;
}


int directory_lookup_pnum(directory dd, const char* name) {
    printf("    dir_lookup_pnum looking for:\"%s\"\n", name);
    if (strcmp(name, "") == 0) {
        return -ENOENT;
    }
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, name) == 0) {
            printf("      found it at index %i\n", ii);
            return dd.ents[ii].pnum;
        }
    }
	// TODO: 
    //free(&dd); // needed?
    printf("      directory_lookup_pnum err\n");
    return -ENOENT;
}

int tree_lookup_pnum(const char* path) {
	printf("  tree_lookup_pnum at path:\"%s\"\n", path);

	slist* dirnames = s_split(path, '/');
    int current_num = 1;

	// Always start in the root directory
    directory current_directory = directory_from_pnum(current_num);
    dirnames = dirnames->next;

    while (dirnames != 0) {
        current_num = directory_lookup_pnum(current_directory, dirnames->data);
        dirnames = dirnames->next;

		pnode* current_pnode = pages_get_node(current_num);
		
		//TODO: fix this code.
		if (S_ISDIR(current_pnode->mode)) {
			// If the thing we just looked up is a directory
		} else {
			// The thing we're looking at is not a directory
			// Return this pnum
		}
		
        current_directory = directory_from_pnum(current_num);

		//TODO: check error handeling
        //assert(curnum >= 1);
        //if (curnum < 2) { return directory_from_pnum(1); }
        //if (!S_ISDIR(current_directory.node->mode)) { 
			//return directory_from_pnum(prevnum); 

		//}
        //assert(S_ISDIR(current.node->mode));
    }
    return current_num;



}

directory directory_from_path(const char* path) {
    slist* dirnames = s_split(path, '/');
    int curnum = 1;
    directory current = directory_from_pnum(curnum);
	// TODO: check error handeling.
    //if (strcmp(dirnames->data, "") == 0) { return current; }
    dirnames = dirnames->next;
    int prevnum = 1;
    while (dirnames != 0) {
        prevnum = curnum;
        curnum = directory_lookup_pnum(current, dirnames->data);
        dirnames = dirnames->next;
        current = directory_from_pnum(curnum);
		// TODO: checkk error handeling
        //assert(curnum >= 1);
        //if (curnum < 2) { return directory_from_pnum(1); }
        if (!S_ISDIR(current.node->mode)) {
			return directory_from_pnum(prevnum); 

		}
        //assert(S_ISDIR(current.node->mode));
    }
    return current;
}

int directory_put_ent(directory dd, const char* name, int pnum) {
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, "") == 0) {
            strcpy(dd.ents[ii].name, name);
            dd.ents[ii].pnum = pnum;
            pnode* node = pages_get_node(pnum);
            node->refs += 1;
            dd.ents[ii].node = node;
            return 0;
        }
    }
    return -ENOENT;
}

int directory_delete(directory dd, const char* name) {
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, name) == 0) {
            dirent current = dd.ents[ii];
            if (dd.ents[ii].node == 0) { printf("node is null\n"); }
            pnode* node = pages_get_node(dd.ents[ii].pnum);
            dd.ents[ii].node = node;
            dd.ents[ii].node->refs -= 1; // for hard links
            strcpy(dd.ents[ii].name, "");
            memset(pages_get_page(dd.ents[ii].pnum), 0, 4096);
            return 0;
        }
    }
    return -ENOENT;
}

slist* directory_list(const char* path) {
    slist* rv = NULL;
    directory dd = directory_from_path(path);
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, "") == 0) { continue; }
        rv = s_cons(dd.ents[ii].name, rv);
    }
    return rv;
}

void print_directory(directory dd) {
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        printf("%s\n", dd.ents[ii].name);
    }
}
