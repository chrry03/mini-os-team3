#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "filesystem.h"
#include "fs_storage.h"
#include "commands.h"

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 64
#define MAX_USER_NAME 64
#define USER_COUNT 2

static const char* VALID_USERS[USER_COUNT] = {
    "root",
    "osmanager"
};

static void trim_newline(char* str) {
    if (str == NULL) return;

    str[strcspn(str, "\n")] = '\0';
}

static char* skip_spaces(char* str) {
    while (str != NULL && *str != '\0' && isspace((unsigned char)*str)) {
        str++;
    }

    return str;
}

static int is_valid_user(const char* username) {
    if (username == NULL) return 0;

    for (int i = 0; i < USER_COUNT; i++) {
        if (strcmp(username, VALID_USERS[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static void print_user_list(void) {
    printf("Users:");

    for (int i = 0; i < USER_COUNT; i++) {
        printf(" %s", VALID_USERS[i]);
    }

    printf("\n");
}

static int login_user(char* current_user, int size) {
    char input_user[MAX_USER_NAME];

    if (current_user == NULL || size <= 0) {
        return 0;
    }

    printf("Mini OS Simulator\n");
    print_user_list();

    while (1) {
        printf("Login: ");

        if (fgets(input_user, sizeof(input_user), stdin) == NULL) {
            return 0;
        }

        trim_newline(input_user);

        if (is_valid_user(input_user)) {
            strncpy(current_user, input_user, size - 1);
            current_user[size - 1] = '\0';
            return 1;
        }

        printf("login: invalid user '%s'\n", input_user);
        print_user_list();
    }
}

static int parse_input(char* input, char* argv[], int max_args) {
    int argc = 0;
    char* p = input;

    while (p != NULL && *p != '\0') {
        p = skip_spaces(p);

        if (*p == '\0') {
            break;
        }

        if (argc >= max_args) {
            printf("mini_os: too many arguments\n");
            return -1;
        }

        if (*p == '"' || *p == '\'') {
            char quote = *p;
            p++;

            argv[argc++] = p;

            while (*p != '\0' && *p != quote) {
                p++;
            }

            if (*p == '\0') {
                printf("mini_os: unmatched quote\n");
                return -1;
            }

            *p = '\0';
            p++;
        } else {
            argv[argc++] = p;

            while (*p != '\0' && !isspace((unsigned char)*p)) {
                p++;
            }

            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }
    }

    return argc;
}

static void print_prompt(FileSystem* fs, const char* current_user) {
    if (fs == NULL || fs->current == NULL) {
        printf("mini_os> ");
        fflush(stdout);
        return;
    }

    update_current_path(fs);

    if (current_user == NULL || current_user[0] == '\0') {
        printf("mini_os:%s> ", fs->current_path);
    } else {
        printf("%s@mini-os:%s> ", current_user, fs->current_path);
    }

    fflush(stdout);
}

static int is_exit_command(const char* command) {
    if (command == NULL) return 0;

    return strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0;
}

static void execute_command(FileSystem* fs, int argc, char* argv[]) {
    if (argc <= 0 || argv[0] == NULL) {
        return;
    }

    if (strcmp(argv[0], "pwd") == 0) {
        command_pwd(fs, argc, argv);
    } else if (strcmp(argv[0], "ls") == 0) {
        command_ls(fs, argc, argv);
    } else if (strcmp(argv[0], "cd") == 0) {
        command_cd(fs, argc, argv);
    } else if (strcmp(argv[0], "mkdir") == 0) {
        command_mkdir(fs, argc, argv);
    } else if (strcmp(argv[0], "cat") == 0) {
        command_cat(fs, argc, argv);
    } else if (strcmp(argv[0], "grep") == 0) {
        command_grep(fs, argc, argv);
    } else if (strcmp(argv[0], "chown") == 0) {
        command_chown(fs, argc, argv);
    } else if (strcmp(argv[0], "mv") == 0) {
        command_mv(fs, argc, argv);
    } else if (strcmp(argv[0], "rm") == 0) {
        command_rm(fs, argc, argv);
    } else if (strcmp(argv[0], "touch") == 0) {
        command_touch(fs, argc, argv);
    } else if (strcmp(argv[0], "tree") == 0) {
        command_tree(fs, argc, argv);
    } else if (strcmp(argv[0], "find") == 0) {
        command_find(fs, argc, argv);
    } else {
        printf("%s: command not found\n", argv[0]);
    }
}

int main(void) {
    FileSystem fs;
    char current_user[MAX_USER_NAME];
    char input[MAX_INPUT_SIZE];
    char input_copy[MAX_INPUT_SIZE];
    char* argv[MAX_ARG_COUNT];
    int argc;

    current_user[0] = '\0';

    init_filesystem(&fs);

    if (load_filesystem(&fs, STORAGE_FILE) != 0) {
        printf("storage: failed to load filesystem state\n");
    }

    if (!login_user(current_user, sizeof(current_user))) {
        destroy_filesystem(&fs);
        return 0;
    }

    printf("Login successful.\n");
    printf("Type 'exit' or 'quit' to save and terminate.\n");

    while (1) {
        print_prompt(&fs, current_user);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        trim_newline(input);

        if (skip_spaces(input)[0] == '\0') {
            continue;
        }

        strncpy(input_copy, input, MAX_INPUT_SIZE - 1);
        input_copy[MAX_INPUT_SIZE - 1] = '\0';

        argc = parse_input(input_copy, argv, MAX_ARG_COUNT);

        if (argc <= 0) {
            continue;
        }

        if (is_exit_command(argv[0])) {
            if (save_filesystem(&fs, STORAGE_FILE) != 0) {
                printf("storage: failed to save filesystem state\n");
            } else {
                printf("filesystem saved.\n");
            }

            break;
        }

        execute_command(&fs, argc, argv);

        if (save_filesystem(&fs, STORAGE_FILE) != 0) {
            printf("storage: warning: failed to save filesystem state\n");
        }
    }

    destroy_filesystem(&fs);

    printf("Mini OS Simulator terminated.\n");

    return 0;
}