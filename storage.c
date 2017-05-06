
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <errno.h>
#include <stdlib.h>

#include "storage.h"
#include "util.h"

void
storage_init(const char* path)
{
    //printf("TODO: Store file system data in: %s\n", path);
    pages_init(path);
    directory_init();    
}

static void*
get_file_data(const char* path) {

    int pnum = tree_lookup_pnum(path);
    pnode* node = pages_get_node(pnum);
    if (node->mode != 040755) {
        return 0;
    }
    return pages_get_page(pnum);
}

int
get_stat(const char* path, struct stat* st)
{
    memset(st, 0, sizeof(struct stat));

    int pnum = tree_lookup_pnum(path);
    printf("pnum: %i\n", pnum);
    if (pnum < 0) {
		printf("returnig: %i\n", -2);
        return -2;
    }

    pnode* node = pages_get_node(pnum);
    st->st_uid  = getuid();
    //printf("uid: %i\n", getuid());
    st->st_mode = node->mode;
    //printf("mode: %i\n", node->mode);
    st->st_size = node->size;
    //printf("returning 0\n");
    return 0;
}

const char*
get_data(const char* path)
{
    void* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    return (char*) dat;
}


int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    int pnum = tree_lookup_pnum(path);
    if (pnum < 0 || offset + size > 4096) { return 0; }
    pnode* node = pages_get_node(pnum);
    if (S_ISDIR(node->mode)) { return -EBADF; }
    void* page = pages_get_page(pnum);
    
    int count = clamp(node->size - offset, 0, size);

    memcpy(buf, page + offset, count);
    return count;
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
    int pnum = tree_lookup_pnum(path);
    pnode* node = pages_get_node(pnum);
    if (pnum < 0 || offset + size > 4096 || node->mode == 040755) { return 0; }
    void* page = pages_get_page(pnum);

    if (node->size < size + offset) { node->size = size + offset; }
    memcpy(page + offset, buf, size);
    return size;
}

int
storage_truncate(const char* path, off_t size)
{
    int pnum = tree_lookup_pnum(path);
    if (pnum < 0) { return -1; }
    pnode* node = pages_get_node(pnum);
    void* page = pages_get_page(pnum);
    if (node-> size < size) { node->size = size; }
    else if (node->size > size) {
        memset(page + node->size, 0, size - node->size);
        node->size = size;
    }
    return 0;
}

static char*
dirname(const char* path)
{
    // turns a path to a file into just the path
    // /users/caleb/hello.txt -> /users/caleb
    printf("Dirname FLAG 0\n");
    slist* dirnames = s_split(path, '/');
    printf("Dirname FLAG 1\n");
    dirnames = dirnames->next;
    size_t inputlen = strlen(path);

    printf("Dirname FLAG 2\n");
    if (dirnames->next == 0) { return "/"; }
    char* rval = malloc(inputlen * sizeof(char));

    printf("Dirname FLAG 3\n");  
    while (dirnames->next != 0) {
        printf("  Dirname while @:\"%s\"\n", dirnames->data);
        strcat(rval, "/");
        strcat(rval, dirnames->data);
        dirnames = dirnames->next;
    }

    printf("Dirname FLAG 4\n");
    return rval;
}

static char*
basename(const char* path)
{
    // turns a path to a file into the file name
    // /users/caleb/hello.txt -> hello.txt
    printf("Basename FLAG 0\n");
    slist* dirnames;
    printf("Basename FLAG !\n");
    dirnames = s_split(path, '/');
    printf("YOU NEVER SEE ME!\n");
    if (strcmp(path, "/") == 0) { return ""; }
    while (1) {
         if (dirnames->next == 0) { break; }
         else { dirnames = dirnames->next; }
    }
    return dirnames->data;
}

int
storage_mknod(const char* path, mode_t mode, dev_t rdev)
{
    printf("mknod point 0\n");
    char* tmp1 = alloca(strlen(path));
    char* tmp2 = alloca(strlen(path));

    strcpy(tmp1, path);
    strcpy(tmp2, path);
    printf("mknod point 0.1\n");
    char* dname = dirname(tmp1);
    printf("mknod point 0.11\n");
    char* name = basename(tmp2);


    printf("mknod point 0.2\n");
    printf("dirname: %s\n", dname);
    directory dd = directory_from_path(dname);
    printf("mknod point 1\n");


    if (dd.node == 0) { return -ENOENT; }
    if (directory_lookup_pnum(dd, name) != -ENOENT) { return -EEXIST; }
    printf("mknod point 2\n");


    int pnum = pages_find_empty();
    pnode* node = pages_get_node(pnum);
    node->refs = 0;
    node->mode = mode;
    return directory_put_ent(dd, name, pnum);
}

int
storage_access(const char* path, int mask)
{
    return tree_lookup_pnum(path);
}

int
storage_unlink(const char* path)
{
    directory dd = directory_from_path(path);
    //printf("in unlink, after directory_from_path\n");
    char* name = basename(path);
    //printf("in unlink, after basename\n");
    return directory_delete(dd, name);
}

int
storage_rename(const char* from, const char* to)
{
    //printf("from: %s, to: %s\n", from, to);
    char* newfilename = basename(to);
    directory newdir = directory_from_path(to);
    int oldpnum = tree_lookup_pnum(from);
    pnode* oldnode = pages_get_node(oldpnum);
    int newpnum = pages_find_empty();

    if (newpnum < 0) { return newpnum; }

    pnode* newnode = pages_get_node(newpnum);
    newnode->mode = oldnode->mode;
    newnode->size = oldnode->size;
    newnode->xtra = oldnode->xtra;

    directory olddir = directory_from_path(from);
    char* oldfilename = basename(from);
    int rv = directory_put_ent(newdir, newfilename, newpnum);
    memcpy(pages_get_page(newpnum), pages_get_page(oldpnum), 4096);
    directory_delete(olddir, oldfilename);
    return rv;
}

int
storage_mkdir(const char* path, mode_t mode)
{
    //mode += 16384; // 40000 in octal
    
    if (!S_ISDIR(mode + 16384)) // 16384 = 40000 in octal 
	{ 
	    printf("bad mode: %u\n", mode);
	    //return -EBADF; 
	}
    return storage_mknod(path, mode + 16384, 0);
}

int
storage_rmdir(const char* path)
{
    directory target_dir = directory_from_path(path);
    char* target_name = alloca(strlen(path));
    strcpy(target_name, path);
    target_name = basename(target_name);
    
    for (int ii = 0; ii < 4096 / sizeof(dirent); ++ii) {
        if (strcmp(target_dir.ents[ii].name, "") != 0) {
            return -1;
        }
    }

    char* parent_path  = alloca(strlen(path));
    strcpy(parent_path, path);
    parent_path = dirname(parent_path);

    directory parent_dir = directory_from_path(parent_path);
    return directory_delete(parent_dir, target_name);
}


// Creates a hard link between "form" and "to". Here, from is 
// and existing path, and to is a new path.
int
storage_link(const char *from, const char *to)
{
	if(/* "to" already exists*/ 0) {
		return -EEXIST;
	}
	
	
	// Get the inode for the origional file
	int old_pnum = tree_lookup_pnum(from);
	
	// Where we want to put the new pnode
	directory target_dir = directory_from_path(to);
	
	// What should we call the new hardlink file?
	const char* name = basename(to);

	// Put a new direent in target_dir pointing to old_pnum
	directory_put_ent(target_dir, name, old_pnum);
}












