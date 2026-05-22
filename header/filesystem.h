#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NAME_SIZE 128
#define PATH_SIZE 1024
#define OWNER_SIZE 64

typedef enum {
    NODE_FILE = 0,
    NODE_DIR = 1
} NodeType;

typedef struct Node {
    char name[NAME_SIZE];
    NodeType type;

    char* content;
    int size;

    char owner[OWNER_SIZE];
    char group[OWNER_SIZE];
    int permission;

    time_t created_at;
    time_t modified_at;

    struct Node* parent;
    struct Node* child;
    struct Node* sibling;
} Node;

typedef struct FileSystem {
    Node* root;
    Node* current;
    char current_path[PATH_SIZE];
    pthread_mutex_t lock;
} FileSystem;

typedef struct ThreadArg {
    FileSystem* fs;
    char target_name[NAME_SIZE];
    char target_path[PATH_SIZE];
    int option_flag;
} ThreadArg;

void init_filesystem(FileSystem* fs);
void destroy_filesystem(FileSystem* fs);

Node* create_node(const char* name, NodeType type);
void add_child(Node* parent, Node* child);
Node* find_child(Node* parent, const char* name);
void remove_child(Node* parent, Node* target);
void free_subtree(Node* node);

Node* resolve_path(FileSystem* fs, const char* path);
Node* resolve_parent_path(FileSystem* fs, const char* path, char* basename);

void update_current_path(FileSystem* fs);
void print_current_path(FileSystem* fs);

int is_directory(Node* node);
int is_file(Node* node);
int has_child(Node* node);
int is_duplicate_name(Node* parent, const char* name);

void update_modified_time(Node* node);
int set_file_content(Node* file, const char* content, int size);

void format_permission(Node* node, char* out);

#endif