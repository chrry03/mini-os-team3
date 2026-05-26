#ifndef FS_STORAGE_H
#define FS_STORAGE_H

#include "filesystem.h"

#define STORAGE_FILE "data/filesystem.dat"

int save_filesystem(FileSystem* fs, const char* filename);
int load_filesystem(FileSystem* fs, const char* filename);

#endif