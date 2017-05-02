#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <bsd/string.h>
#include <assert.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "slist.h"

// implementation for: man 2 access
// Checks if a file exists.
int
nocabfs_access(const char *path, int mask)
{
    printf("access(%s, %04o)\n", path, mask);
    if (/*path doesn't exist*/!storage_access(path, mask)) {
        return -ENOENT;
    }
    else if (/*requested permission isn't available*/0) {
        return -EACCES;
    }
    return 0;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nocabfs_getattr(const char *path, struct stat *st)
{
    printf("getattr(%s)\n", path);
    int rv = get_stat(path, st);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        //printf("returning %i\n", rv);
        return rv; //0;
    }
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nocabfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    struct stat st;

    printf("readdir(%s)\n", path);
    
    slist* contents = directory_list(path);

    get_stat(path, &st);

    //get_stat("/", &st);
    // filler is a callback that adds one item to the result
    // it will return non-zero when the buffer is full
    filler(buf, ".", &st, 0);
    
    while (contents) {
        filler(buf, contents->data, &st, 0);
        contents = contents->next;
    }

    //get_stat("/hello.txt", &st);
    //filler(buf, "hello.txt", &st, 0);

    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nocabfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    printf("mknod(%s, %04o)\n", path, mode);
    return storage_mknod(path, mode, rdev);//-1;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nocabfs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir(%s)\n", path);
     
    if (/*failedrv != */0) {
        return -errno;
    }
    return storage_mkdir(path, mode);
}

int
nocabfs_unlink(const char *path)
{
    printf("unlink(%s)\n", path);
    return storage_unlink(path);
/* 
    if (0) {
        return -errno;
    }
    return 0;//-1;
*/
}

int
nocabfs_rmdir(const char *path)
{
    printf("rmdir(%s)\n", path);
    
    if (0) {
        return -errno;
    }
    return storage_rmdir(path);//-1;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nocabfs_rename(const char *from, const char *to)
{
    printf("rename(%sr=> %s)\n", from, to);
    return storage_rename(from, to);
}

int
nocabfs_chmod(const char *path, mode_t mode)
{
    printf("chmod(%s, %04o)\n", path, mode);
    return -1;
}

int
nocabfs_truncate(const char *path, off_t size)
{
    printf("truncate(%s, %ld bytes)\n", path, size);
    return storage_truncate(path, size);
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nocabfs_open(const char *path, struct fuse_file_info *fi)
{
    printf("open(%s)\n", path);
    return 0;
}

// Actually read data
int
nocabfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read(%s, %ld bytes, @%ld)\n", path, size, offset);
    return storage_read(path, buf, size, offset);
}

// Actually write data
int
nocabfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write(%s, %ld bytes, @%ld)\n", path, size, offset);
    return storage_write(path, buf, size, offset);//-1;
}

void
nocabfs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nocabfs_access;
    ops->getattr  = nocabfs_getattr;
    ops->readdir  = nocabfs_readdir;
    ops->mknod    = nocabfs_mknod;
    ops->mkdir    = nocabfs_mkdir;
    ops->unlink   = nocabfs_unlink;
    ops->rmdir    = nocabfs_rmdir;
    ops->rename   = nocabfs_rename;
    ops->chmod    = nocabfs_chmod;
    ops->truncate = nocabfs_truncate;
    ops->open	    = nocabfs_open;
    ops->read     = nocabfs_read;
    ops->write    = nocabfs_write;
};

struct fuse_operations nocabfs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2);// && argc < 5);
    storage_init(argv[--argc]);
    nocabfs_init_ops(&nocabfs_ops);
    return fuse_main(argc, argv, &nocabfs_ops, NULL);
}

