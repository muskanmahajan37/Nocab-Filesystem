
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

// Gets the page/data refrenced at the specified payt
static void*
get_file_data(const char* path) {

    int pnum = tree_lookup_pnum(path);
    pnode* node = pages_get_node(pnum);
    if (node->mode != 040755) {
        return 0;
    }
    return pages_get_page(pnum);
}

// Gets stats, or meta data about the file at path. Stores 
// the information in the stat struct.
int
get_stat(const char* path, struct stat* st)
{
    memset(st, 0, sizeof(struct stat));

    int pnum = tree_lookup_pnum(path);
    printf("pnum: %i\n", pnum);
    if (pnum < 0) {
        return -ENOENT;
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

// returns the data refrenced by path, but casted as a 
// char*
const char*
get_data(const char* path)
{
    void* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    return (char*) dat;
}

// coppies the data from path, starting at offset, into the buff.
// Will not coppy more than specified size, or past end of file
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

// coppies data from the buff, into the file at path starting at
// the offset. Will not coppy more than specified size, or past
// the end of the file. 
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

// 0s out add the data in the file at path, past the specified 
// size.
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

// converts a valid path into the path to the parent directory.
// EG:   /users/caleb/hello.txt -> /users/caleb
static char*
dirname(const char* path)
{
    slist* dirnames = s_split(path, '/');
    dirnames = dirnames->next;
    size_t inputlen = strlen(path);

    if (dirnames->next == 0) { return "/"; }
    char* rval = malloc(inputlen * sizeof(char));

    while (dirnames->next != 0) {
        strcat(rval, "/");
        strcat(rval, dirnames->data);
        dirnames = dirnames->next;
    }

    printf("Dirname FLAG 4\n");
    return rval;
}

// converts a valid path into the name of the file.
// EG:   /users/caleb/hello.txt -> hello.txt
static char*
basename(const char* path)
{
    slist* dirnames;
    dirnames = s_split(path, '/');
    if (strcmp(path, "/") == 0) { return ""; }
    while (1) {
         if (dirnames->next == 0) { break; }
         else { dirnames = dirnames->next; }
    }
    return dirnames->data;
}

// adds a new file, with name and location specified by path,
// with provided mode.
int
storage_mknod(const char* path, mode_t mode, dev_t rdev)
{
    char* tmp1 = alloca(strlen(path));
    char* tmp2 = alloca(strlen(path));

    strcpy(tmp1, path);
    strcpy(tmp2, path);
    char* dname = dirname(tmp1);
    char* name = basename(tmp2);

    directory dd = directory_from_path(dname);

    if (dd.node == 0) { return -ENOENT; }
    if (directory_lookup_pnum(dd, name) != -ENOENT) { return -EEXIST; }

    int pnum = pages_find_empty();
    pnode* node = pages_get_node(pnum);
    node->refs = 0;
    node->mode = mode;
    return directory_put_ent(dd, name, pnum);
}

// Retrieves the pnum from the file at path
int
storage_access(const char* path, int mask)
{
    return tree_lookup_pnum(path);
}

// removes te refrence, specified by path, to the file. If this
// was the last refrence the file is deleted and memory is returned.
int
storage_unlink(const char* path)
{
    directory dd = directory_from_path(path);
    char* name = basename(path);
    return directory_delete(dd, name);
}

// moves a file 
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

// creates a new directory at specified path
int
storage_mkdir(const char* path, mode_t mode)
{
    //mode += 16384; // 40000 in octal
    
	// TODO: double check this error handeling
    if (!S_ISDIR(mode + 16384)) // 16384 = 40000 in octal 
	{ 
	    printf("bad mode: %u\n", mode);
	    return -EBADF; 
	}
    return storage_mknod(path, mode + 16384, 0);
}

// removes the specified directory
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

// creats a softlink between the paths
int 
storage_symlink(const char* from, const char* to)
{
	directory dir = directory_from_path(to);
	const char* name = basename(to);

    if (dir.node == 0) { return -ENOENT; }
    if (directory_lookup_pnum(dir, name) != -ENOENT) { return -EEXIST; }

    int pnum = pages_find_empty();
    pnode* node = pages_get_node(pnum);
    node->refs = 0;
    node->mode = 41453;//S_IFLNK + 755;  // <----- mark it as a symlink
	
	int from_pnum = tree_lookup_pnum(from);
	node->xtra = from_pnum;  // store the pnum of the origional file here.
	
	// Store the path of from into the page @ to
	void* page = pages_get_page(pnum);
	memcpy(page, from, sizeof page);

    return directory_put_ent(dir, name, pnum);
	
}


// TODO: fix this
// reads the link, specified by paht, and coppies the next path 
// into buff.
int
storage_readlink(const char* path, char* buff, size_t size)
{
	int link_pnum = tree_lookup_pnum(path);
	pnode* link_node = pages_get_node(link_pnum);	

	int next_pnum = link_node->xtra;
	//pnode* next_node = pages_get_node(next_pnum);
	void* page = pages_get_page(next_pnum);
	
	memcpy(buff, page, size);
	return size;
}









