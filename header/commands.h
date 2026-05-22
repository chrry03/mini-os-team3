#ifndef COMMANDS_H
#define COMMANDS_H

#include "filesystem.h"

void command_pwd(FileSystem* fs);

void command_ls(FileSystem* fs, int argc, char* argv[]);
void command_cd(FileSystem* fs, int argc, char* argv[]);

void command_mkdir(FileSystem* fs, int argc, char* argv[]);
void command_cat(FileSystem* fs, int argc, char* argv[]);

void command_grep(FileSystem* fs, int argc, char* argv[]);
void command_chown(FileSystem* fs, int argc, char* argv[]);

void command_mv(FileSystem* fs, int argc, char* argv[]);
void command_rm(FileSystem* fs, int argc, char* argv[]);

#endif