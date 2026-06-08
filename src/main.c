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
#define USER_COUNT 5

static const char* VALID_USERS[USER_COUNT] = {
    "root",
    "user1",
    "user2",
    "user3",
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

static Node* ensure_child_directory(Node* parent, const char* dirname) {
    if (parent == NULL || dirname == NULL) {
        return NULL;
    }

    Node* child = find_child(parent, (char*)dirname);

    if (child != NULL) {
        if (child->type != NODE_DIR) {
            return NULL;
        }

        return child;
    }

    Node* new_dir = create_node((char*)dirname, NODE_DIR);

    if (new_dir == NULL) {
        return NULL;
    }

    add_child(parent, new_dir);

    return new_dir;
}

static int move_to_user_home(FileSystem* fs, const char* current_user) {
    if (fs == NULL || fs->root == NULL || current_user == NULL) {
        return 0;
    }

    Node* home_dir = NULL;

    if (strcmp(current_user, "root") == 0) {
        home_dir = ensure_child_directory(fs->root, "root");

        if (home_dir == NULL) {
            printf("login: failed to prepare home directory for root\n");
            return 0;
        }
    } else {
        Node* home_parent = ensure_child_directory(fs->root, "home");

        if (home_parent == NULL) {
            printf("login: failed to prepare /home directory\n");
            return 0;
        }

        home_dir = ensure_child_directory(home_parent, current_user);

        if (home_dir == NULL) {
            printf("login: failed to prepare home directory for %s\n", current_user);
            return 0;
        }
    }

    fs->current = home_dir;
    update_current_path(fs);

    return 1;
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

static void get_home_path(const char* current_user, char* home_path, int size) {
    if (home_path == NULL || size <= 0) {
        return;
    }

    home_path[0] = '\0';

    if (current_user == NULL || current_user[0] == '\0') {
        return;
    }

    if (strcmp(current_user, "root") == 0) {
        strncpy(home_path, "/root", size - 1);
        home_path[size - 1] = '\0';
    } else {
        snprintf(home_path, size, "/home/%s", current_user);
    }
}

static void make_display_path(const char* current_path, const char* current_user, char* display_path, int size) {
    char home_path[PATH_SIZE];

    if (display_path == NULL || size <= 0) {
        return;
    }

    display_path[0] = '\0';

    if (current_path == NULL || current_path[0] == '\0') {
        strncpy(display_path, "/", size - 1);
        display_path[size - 1] = '\0';
        return;
    }

    get_home_path(current_user, home_path, sizeof(home_path));

    if (home_path[0] != '\0') {
        int home_len = (int)strlen(home_path);

        if (strcmp(current_path, home_path) == 0) {
            strncpy(display_path, "~", size - 1);
            display_path[size - 1] = '\0';
            return;
        }

        if (strncmp(current_path, home_path, home_len) == 0 && current_path[home_len] == '/') {
            snprintf(display_path, size, "~%s", current_path + home_len);
            return;
        }
    }

    strncpy(display_path, current_path, size - 1);
    display_path[size - 1] = '\0';
}

static void print_prompt(FileSystem* fs, const char* current_user) {
    char display_path[PATH_SIZE];

    if (fs == NULL || fs->current == NULL) {
        printf("mini_os> ");
        fflush(stdout);
        return;
    }

    update_current_path(fs);
    make_display_path(fs->current_path, current_user, display_path, sizeof(display_path));

    if (current_user == NULL || current_user[0] == '\0') {
        printf("mini_os:%s> ", display_path);
    } else {
        printf("%s@mini-os:%s> ", current_user, display_path);
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

    set_current_user(current_user);

    if (!move_to_user_home(&fs, current_user)) {
        destroy_filesystem(&fs);
        return 0;
    }

    if (save_filesystem(&fs, STORAGE_FILE) != 0) {
        printf("storage: warning: failed to save filesystem state\n");
    }

    printf("Login successful.\n");
    printf("Type 'exit' or 'quit' to save and terminate.\n");

    while (1) {
        print_prompt(&fs, current_user);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");

            if (save_filesystem(&fs, STORAGE_FILE) != 0) {
                printf("storage: failed to save filesystem state\n");
            } else {
                printf("filesystem saved.\n");
            }

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