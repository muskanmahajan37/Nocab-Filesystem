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
    //printf("rn size: %i, rn refs: %i, rn mode: %i\n", rn->size, rn->refs, rn->mode);
    if (rn->refs == 0) {
        //printf("root node ref count: %i\n", rn->refs);
        rn->size = 4096;
        rn->refs = 1;
        rn->mode = 040755;//mode indicates directory
        rn->xtra = 0;// what should this be?
        //printf("set root node fields in directory_init\n");
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
    //if (S_ISDIR(rv.node->mode)) {
        return rv;
    //}
    //return NULL;
}


int directory_lookup_pnum(directory dd, const char* name) {
    printf("    dir_lookup_pnum looking for:\"%s\"\n", name);
    if (strcmp(name, "") == 0) {
        printf("    empty string, returning error\n");
        return -ENOENT;
    }
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, name) == 0) {
            printf("      found it at index %i\n", ii);
            return dd.ents[ii].pnum;
        }
    }
    //free(&dd); // needed?
    printf("      directory_lookup_pnum err\n");
    return -ENOENT;
}

/*
static char*
basename(const char* path)
{
    // turns a path to a file into the file name
    // /users/caleb/hello.txt -> hello.txt
    slist* dirnames = s_split(path, '/');
    if (strcmp(path, "/") == 0) { return ""; }
    while (1) {
         if (dirnames->next == 0) { break; }
         else { dirnames = dirnames->next; }
    }
    return dirnames->data;
}

static char*
dirname(const char* path)
{
    // turns a path to a file into just the path
    // /users/caleb/hello.txt -> /users/caleb

	printf("    dirname looking @:\"%s\"\n", path);
    slist* dirnames = s_split(path, '/');

	printf("    dirname flag 1");
    dirnames = dirnames->next;
    size_t inputlen = strlen(path);

    if (dirnames->next == 0) { return "/"; }

	printf("    dirname flag 2");
    char* rval = malloc(inputlen * sizeof(char)); 

	printf("    dirname flag 3");
    while (dirnames->next != 0) {
        strcat(rval, "/");
        strcat(rval, dirnames->data);
    }
	printf("    dirname returns \n");
    return rval;
}
*/


int tree_lookup_pnum(const char* path) {
	printf("  tree_lookup_pnum at path:\"%s\"\n", path);

	
	slist* dirnames = s_split(path, '/');
    int current_num = 1;
	// Always start in the root directory
    directory current_directory = directory_from_pnum(current_num);
    dirnames = dirnames->next;
    //int prevnum = 1;

    while (dirnames != 0) {
        //prevnum = curnum;
        current_num = directory_lookup_pnum(current_directory, dirnames->data);
        dirnames = dirnames->next;

		pnode* current_pnode = pages_get_node(current_num);
		
		if (S_ISDIR(current_pnode->mode)) {
			// If the thing we just looked up is a directory
		} else {
			// The thing we're looking at is not a directory
			// Return this pnum
		}
		
        current_directory = directory_from_pnum(current_num);
        //assert(curnum >= 1);
        //if (curnum < 2) { return directory_from_pnum(1); }
        //if (!S_ISDIR(current_directory.node->mode)) { 
			//return directory_from_pnum(prevnum); 

		//}
        //assert(S_ISDIR(current.node->mode));
    }
    return current_num;



	/*
    if (strcmp(path, "/") == 0) { return 1; }
	
    char* tmp1 = alloca(strlen(path));
    char* tmp2 = alloca(strlen(path));

    strcpy(tmp1, path);
    strcpy(tmp2, path);
    printf("mknod point 0.1\n");
    //char* parent_dir_path = dirname(tmp1);
    char* target_name = basename(tmp2);


	directory dd = directory_from_path(path);

	/* 
	//Go to the end of the path
	parent_dir_path = dirname(path);
	printf("    tlp flag 1\n");
	directory dd = directory_from_path(parent_dir_path);
	printf("    tlp flag 2\n");
	// Look for something with the specified name
	target_name = basename(path);
	printf("    tlp flag 3\n");
	
	* /

	// loop through the dirent array
	for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
		printf("      tlp flag LOOP\n");
		if (strcmp(dd.ents[ii].name, target_name) == 0) {
			//printf("    tree_lookup_dir found pnum:%i\n", dd.ents[ii].pnum);
			printf("    tlp loop exit 1\n");
			return dd.ents[ii].pnum;
		}
	}
	printf("    tree_lookup_dir returning error\n");
	return -ENOENT;
	*/

	/*
    directory dd = directory_from_path(path);
    slist* dirnames = s_split(path, '/');
	dirnames = dirnames->next; // the first element is always empty string
    int rval = -1;
	//char* base_name = basename(path);
	//rval = directory_lookup_pnum(dd, base_name);
    
	while (dirnames) {
		printf("    looping @ string:\"%s\"\n", dirnames->data);
        rval = directory_lookup_pnum(dd, dirnames->data);
        dirnames = dirnames->next;
    }
	
    //printf("tree_lookup_pnum returning %i\n", rval);
    return rval;
	*/
}

directory directory_from_path(const char* path) {
    //printf("dir_from_path point 0\n");
	//printf("  directory_from_path looking @path:\"%s\"\n", path);
    slist* dirnames = s_split(path, '/');
    //printf("dir_from_path point 1\n");
    int curnum = 1;
    directory current = directory_from_pnum(curnum);
    //if (strcmp(dirnames->data, "") == 0) { return current; }
    dirnames = dirnames->next;
    int prevnum = 1;
    while (dirnames != 0) {
        //printf("dirnames->data: %s, current.pnum: %i\n", dirnames->data, current.pnum);
        prevnum = curnum;
        curnum = directory_lookup_pnum(current, dirnames->data);
        dirnames = dirnames->next;
        current = directory_from_pnum(curnum);
        //assert(curnum >= 1);
        //if (curnum < 2) { return directory_from_pnum(1); }
        if (!S_ISDIR(current.node->mode)) { 
			//printf("  directory_from_path returning dirnames1:\"%s\"\n", dirnames->data);
			return directory_from_pnum(prevnum); 

		}
        //assert(S_ISDIR(current.node->mode));
    }
	//printf("  directory_from_path returning dirnames:\"%s\"\n", dirnames->data);
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
    //printf("directory_delete\n");
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(dd.ents[ii].name, name) == 0) {
            //printf("found the file in directory_delete\n");
            dirent current = dd.ents[ii];
            //printf("directory_delete after current\n");
            //printf("node refs: %i\n", current.node->refs);
            if (dd.ents[ii].node == 0) { printf("node is null\n"); }
            //printf("node is something\n");
            pnode* node = pages_get_node(dd.ents[ii].pnum);
            dd.ents[ii].node = node;
            dd.ents[ii].node->refs = 0; // -= 1 for hard links
            //printf("directory_delete after node refs = 0\n");
            strcpy(dd.ents[ii].name, "");
            //printf("directory_delete returning 0\n");
            memset(pages_get_page(dd.ents[ii].pnum), 0, 4096);
            return 0;
        }
    }
    //printf("no file found to delete\n");
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
