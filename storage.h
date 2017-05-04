#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "directory.h"

void        storage_init(const char* path);
int         get_stat(const char* path, struct stat* st);
const char* get_data(const char* path);
int         storage_read(const char* path, char* buf, size_t size, off_t offset);
int         storage_write(const char* path, const char* buf, size_t size, off_t offset);
int         storage_truncate(const char* path, off_t size);
int         storage_mknod(const char* path, mode_t mode, dev_t rdev);
int         storage_access(const char* path, int mask);
int         storage_unlink(const char* path);
int         storage_rename(const char* from, const char* to);
int         storage_mkdir(const char* path, mode_t mode);
int         storage_rmdir(const char* path);

#endif
