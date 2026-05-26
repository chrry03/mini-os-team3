#include <stdio.h>
#include <string.h>
#include "filesystem.h"
#include "commands.h"

static int is_valid_cd_argument_count(int argc) 
{
    if (argc <= 2) 
    {
        return 1;
    }

    return 0;
}

static const char* get_cd_target_path(int argc, char* argv[]) 
{
    if (argc == 1) 
    {
        return "/";
    }

    if (argv == NULL || argv[1] == NULL) 
    {
        return "/";
    }

    return argv[1];
}

static int move_to_target_directory(FileSystem* fs, const char* path) 
{
    Node* target;

    if (fs == NULL || path == NULL) 
    {
        printf("cd: filesystem error\n");
        return -1;
    }

    target = resolve_path(fs, path);

    if (target == NULL) 
    {
        printf("cd: %s: No such file or directory\n", path);
        return -1;
    }

    if (target->type != NODE_DIR) 
    {
        printf("cd: %s: Not a directory\n", path);
        return -1;
    }

    fs->current = target;
    update_current_path(fs);

    return 0;
}

void command_cd(FileSystem* fs, int argc, char* argv[]) 
{
    const char* target_path;

    if (fs == NULL) 
    {
        printf("cd: filesystem error\n");
        return;
    }

    if (!is_valid_cd_argument_count(argc)) 
    {
        printf("cd: too many arguments\n");
        return;
    }

    target_path = get_cd_target_path(argc, argv);

    pthread_mutex_lock(&(fs->lock));
    move_to_target_directory(fs, target_path);
    pthread_mutex_unlock(&(fs->lock));
}