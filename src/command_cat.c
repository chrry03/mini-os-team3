#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#include "commands.h"

#define CAT_BUFFER_SIZE 8192 //ㅇㅇ

static void print_file_content(Node* file) {
    if (file->content == NULL) {
        return;
    }

    printf("%s", file->content);

    int len = strlen(file->content);
    if (len > 0 && file->content[len - 1] != '\n') {
        printf("\n");
    }
}

static void print_file_content_with_line_number(Node* file) {
    if (file->content == NULL) {
        return;
    }

    int line_number = 1;
    int is_line_start = 1;

    for (int i = 0; file->content[i] != '\0'; i++) {
        if (is_line_start) {
            printf("%6d  ", line_number++);
            is_line_start = 0;
        }

        putchar(file->content[i]);

        if (file->content[i] == '\n') {
            is_line_start = 1;
        }
    }

    int len = strlen(file->content);
    if (len > 0 && file->content[len - 1] != '\n') {
        printf("\n");
    }
}

static void cat_read_file(FileSystem* fs, const char* path, int line_number_option) {
    Node* target = resolve_path(fs, path);

    if (target == NULL) {
        printf("cat: %s: No such file\n", path);
        return;
    }

    if (!is_file(target)) {
        printf("cat: %s: Is a directory\n", path);
        return;
    }

    if (line_number_option) {
        print_file_content_with_line_number(target);
    } else {
        print_file_content(target);
    }
}

static void cat_write_file(FileSystem* fs, const char* path) {
    char basename[NAME_SIZE];
    Node* parent = resolve_parent_path(fs, path, basename);

    if (parent == NULL) {
        printf("cat: %s: No such directory\n", path);
        return;
    }

    if (!is_directory(parent)) {
        printf("cat: %s: Parent is not a directory\n", path);
        return;
    }

    if (strlen(basename) == 0) {
        printf("cat: invalid file name\n");
        return;
    }

    Node* file = find_child(parent, basename);

    if (file != NULL && !is_file(file)) {
        printf("cat: %s: Is a directory\n", path);
        return;
    }

    if (file == NULL) {
        file = create_node(basename, NODE_FILE);
        if (file == NULL) {
            printf("cat: failed to create file '%s'\n", path);
            return;
        }

        add_child(parent, file);
        update_modified_time(parent);
    }

    char buffer[CAT_BUFFER_SIZE];
    char line[1024];

    buffer[0] = '\0';

    printf("Enter file content. Type EOF or . on a single line to finish.\n");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strcmp(line, "EOF\n") == 0 || strcmp(line, ".\n") == 0 ||
            strcmp(line, "EOF") == 0 || strcmp(line, ".") == 0) {
            break;
        }

        if (strlen(buffer) + strlen(line) >= CAT_BUFFER_SIZE - 1) {
            printf("cat: content is too large. Input stopped.\n");
            break;
        }

        strcat(buffer, line);
    }

    pthread_mutex_lock(&fs->lock);

    if (set_file_content(file, buffer, (int)strlen(buffer)) != 0) {
        printf("cat: failed to write content to '%s'\n", path);
    } else {
        update_modified_time(file);
    }

    pthread_mutex_unlock(&fs->lock);
}

void command_cat(FileSystem* fs, int argc, char* argv[]) {
    if (argc < 2) {
        printf("cat: missing operand\n");
        return;
    }

    if (strcmp(argv[1], ">") == 0) {
        if (argc < 3) {
            printf("cat: missing file operand after '>'\n");
            return;
        }

        cat_write_file(fs, argv[2]);
        return;
    }

    if (strcmp(argv[1], "-n") == 0) {
        if (argc < 3) {
            printf("cat: missing file operand\n");
            return;
        }

        for (int i = 2; i < argc; i++) {
            cat_read_file(fs, argv[i], 1);
        }

        return;
    }

    for (int i = 1; i < argc; i++) {
        cat_read_file(fs, argv[i], 0);
    }
}