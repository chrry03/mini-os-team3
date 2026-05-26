#include <stdio.h>
#include "filesystem.h"
#include "commands.h"

void command_pwd(FileSystem* fs, int argc, char* argv[]) {
    if (fs == NULL) {
        printf("pwd: filesystem is not initialized\n");
        return;
    }

    if (fs->current == NULL) {
        printf("pwd: current directory is not set\n");
        return;
    }

    if (argc > 1) {
        printf("pwd: too many arguments\n");
        return;
    }

    print_current_path(fs);
}