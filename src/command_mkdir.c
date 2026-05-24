#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "filesystem.h"
#include "commands.h"

static void create_directory_normal(FileSystem* fs, const char* path) {
    char basename[NAME_SIZE];

    Node* parent = resolve_parent_path(fs, path, basename);

    if (parent == NULL) {
        printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
        return;
    }

    if (!is_directory(parent)) {
        printf("mkdir: cannot create directory '%s': Parent is not a directory\n", path);
        return;
    }

    if (strlen(basename) == 0) {
        printf("mkdir: invalid directory name\n");
        return;
    }

    if (is_duplicate_name(parent, basename)) {
        printf("mkdir: cannot create directory '%s': File exists\n", path);
        return;
    }

    Node* new_dir = create_node(basename, NODE_DIR);
    if (new_dir == NULL) {
        printf("mkdir: failed to create directory '%s'\n", path);
        return;
    }

    add_child(parent, new_dir);
    update_modified_time(parent);
}

static void create_directory_p(FileSystem* fs, const char* path) {
    char path_copy[PATH_SIZE];

    strncpy(path_copy, path, PATH_SIZE - 1);
    path_copy[PATH_SIZE - 1] = '\0';

    Node* current;

    if (path[0] == '/') {
        current = fs->root;
    } else {
        current = fs->current;
    }

    char* token = strtok(path_copy, "/");

    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }

        if (strcmp(token, "..") == 0) {
            if (current->parent != NULL) {
                current = current->parent;
            }
            token = strtok(NULL, "/");
            continue;
        }

        Node* next = find_child(current, token);

        if (next != NULL) {
            if (!is_directory(next)) {
                printf("mkdir: cannot create directory '%s': Not a directory\n", path);
                return;
            }
            current = next;
        } else {
            Node* new_dir = create_node(token, NODE_DIR);
            if (new_dir == NULL) {
                printf("mkdir: failed to create directory '%s'\n", token);
                return;
            }

            add_child(current, new_dir);
            update_modified_time(current);
            current = new_dir;
        }

        token = strtok(NULL, "/");
    }
}

static void* mkdir_thread_func(void* arg) {
    ThreadArg* t_arg = (ThreadArg*)arg;
    FileSystem* fs = t_arg->fs;

    pthread_mutex_lock(&fs->lock);

    if (t_arg->option_flag == 1) {
        create_directory_p(fs, t_arg->target_path);
    } else {
        create_directory_normal(fs, t_arg->target_path);
    }

    pthread_mutex_unlock(&fs->lock);

    return NULL;
}

void command_mkdir(FileSystem* fs, int argc, char* argv[]) {
    int option_p = 0;
    int start_index = 1;

    if (argc < 2) {
        printf("mkdir: missing operand\n");
        return;
    }

    if (strcmp(argv[1], "-p") == 0) {
        option_p = 1;
        start_index = 2;

        if (argc < 3) {
            printf("mkdir: missing operand\n");
            return;
        }
    }

    int target_count = argc - start_index;

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * target_count);
    ThreadArg* args = (ThreadArg*)malloc(sizeof(ThreadArg) * target_count);
    int* created = (int*)malloc(sizeof(int) * target_count);

    if (threads == NULL || args == NULL || created == NULL) {
        printf("mkdir: memory allocation failed\n");
        free(threads);
        free(args);
        free(created);
        return;
    }

    for (int i = 0; i < target_count; i++) {
        created[i] = 0;

        args[i].fs = fs;
        args[i].option_flag = option_p;

        strncpy(args[i].target_path, argv[start_index + i], PATH_SIZE - 1);
        args[i].target_path[PATH_SIZE - 1] = '\0';

        strncpy(args[i].target_name, argv[start_index + i], NAME_SIZE - 1);
        args[i].target_name[NAME_SIZE - 1] = '\0';

        if (pthread_create(&threads[i], NULL, mkdir_thread_func, &args[i]) != 0) {
            printf("mkdir: failed to create thread for '%s'\n", argv[start_index + i]);
        } else {
            created[i] = 1;
        }
    }

    for (int i = 0; i < target_count; i++) {
        if (created[i]) {
            pthread_join(threads[i], NULL);
        }
    }

    free(threads);
    free(args);
    free(created);
}