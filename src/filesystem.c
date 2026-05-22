#include "filesystem.h"

/* filesystem.h에서 선언했던 공통함수들 구현 */
void init_filesystem(FileSystem* fs) {
    if (fs == NULL) return;

    fs->root = create_node("/", NODE_DIR);
    fs->current = fs->root;

    strncpy(fs->current_path, "/", PATH_SIZE - 1);
    fs->current_path[PATH_SIZE - 1] = '\0';

    pthread_mutex_init(&fs->lock, NULL);
}

void destroy_filesystem(FileSystem* fs) {
    if (fs == NULL) return;

    free_subtree(fs->root);

    fs->root = NULL;
    fs->current = NULL;
    fs->current_path[0] = '\0';

    pthread_mutex_destroy(&fs->lock);
}

Node* create_node(const char* name, NodeType type) {
    Node* node = (Node*)malloc(sizeof(Node));

    if (node == NULL) {
        printf("filesystem: memory allocation failed\n");
        return NULL;
    }

    strncpy(node->name, name, NAME_SIZE - 1);
    node->name[NAME_SIZE - 1] = '\0';

    node->type = type;
    node->content = NULL;
    node->size = 0;

    strncpy(node->owner, "osmanager", OWNER_SIZE - 1);
    node->owner[OWNER_SIZE - 1] = '\0';

    strncpy(node->group, "osgroup", OWNER_SIZE - 1);
    node->group[OWNER_SIZE - 1] = '\0';

    node->permission = (type == NODE_DIR) ? 755 : 644;

    node->created_at = time(NULL);
    node->modified_at = node->created_at;

    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;

    return node;
}

void add_child(Node* parent, Node* child) {
    if (parent == NULL || child == NULL) return;
    if (parent->type != NODE_DIR) return;

    child->parent = parent;
    child->sibling = NULL;

    if (parent->child == NULL) {
        parent->child = child;
    } else {
        Node* cur = parent->child;

        while (cur->sibling != NULL) {
            cur = cur->sibling;
        }

        cur->sibling = child;
    }

    update_modified_time(parent);
}

Node* find_child(Node* parent, const char* name) {
    if (parent == NULL || name == NULL) return NULL;
    if (parent->type != NODE_DIR) return NULL;

    Node* cur = parent->child;

    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }

        cur = cur->sibling;
    }

    return NULL;
}

void remove_child(Node* parent, Node* target) {
    if (parent == NULL || target == NULL) return;
    if (parent->type != NODE_DIR) return;

    Node* cur = parent->child;
    Node* prev = NULL;

    while (cur != NULL) {
        if (cur == target) {
            if (prev == NULL) {
                parent->child = cur->sibling;
            } else {
                prev->sibling = cur->sibling;
            }

            cur->parent = NULL;
            cur->sibling = NULL;

            update_modified_time(parent);
            return;
        }

        prev = cur;
        cur = cur->sibling;
    }
}

void free_subtree(Node* node) {
    if (node == NULL) return;

    Node* child = node->child;

    while (child != NULL) {
        Node* next = child->sibling;
        free_subtree(child);
        child = next;
    }

    if (node->content != NULL) {
        free(node->content);
        node->content = NULL;
    }

    free(node);
}

Node* resolve_path(FileSystem* fs, const char* path) {
    if (fs == NULL || path == NULL || path[0] == '\0') {
        return NULL;
    }

    if (strcmp(path, "/") == 0) {
        return fs->root;
    }

    Node* cur = (path[0] == '/') ? fs->root : fs->current;

    char buffer[PATH_SIZE];
    strncpy(buffer, path, PATH_SIZE - 1);
    buffer[PATH_SIZE - 1] = '\0';

    char* saveptr = NULL;
    char* token = strtok_r(buffer, "/", &saveptr);

    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            token = strtok_r(NULL, "/", &saveptr);
            continue;
        }

        if (strcmp(token, "..") == 0) {
            if (cur->parent != NULL) {
                cur = cur->parent;
            }

            token = strtok_r(NULL, "/", &saveptr);
            continue;
        }

        Node* next = find_child(cur, token);

        if (next == NULL) {
            return NULL;
        }

        cur = next;
        token = strtok_r(NULL, "/", &saveptr);
    }

    return cur;
}

Node* resolve_parent_path(FileSystem* fs, const char* path, char* basename) {
    if (fs == NULL || path == NULL || basename == NULL) return NULL;

    char buffer[PATH_SIZE];
    strncpy(buffer, path, PATH_SIZE - 1);
    buffer[PATH_SIZE - 1] = '\0';

    char* slash = strrchr(buffer, '/');

    if (slash == NULL) {
        strncpy(basename, buffer, NAME_SIZE - 1);
        basename[NAME_SIZE - 1] = '\0';
        return fs->current;
    }

    strncpy(basename, slash + 1, NAME_SIZE - 1);
    basename[NAME_SIZE - 1] = '\0';

    if (slash == buffer) {
        return fs->root;
    }

    *slash = '\0';

    return resolve_path(fs, buffer);
}

void update_current_path(FileSystem* fs) {
    if (fs == NULL || fs->current == NULL) return;

    if (fs->current == fs->root) {
        strncpy(fs->current_path, "/", PATH_SIZE - 1);
        fs->current_path[PATH_SIZE - 1] = '\0';
        return;
    }

    Node* stack[128];
    int top = 0;

    Node* cur = fs->current;

    while (cur != NULL && cur != fs->root && top < 128) {
        stack[top++] = cur;
        cur = cur->parent;
    }

    char result[PATH_SIZE] = "";
    char temp[NAME_SIZE + 2];

    for (int i = top - 1; i >= 0; i--) {
        snprintf(temp, sizeof(temp), "/%s", stack[i]->name);
        strncat(result, temp, PATH_SIZE - strlen(result) - 1);
    }

    if (result[0] == '\0') {
        strncpy(result, "/", PATH_SIZE - 1);
        result[PATH_SIZE - 1] = '\0';
    }

    strncpy(fs->current_path, result, PATH_SIZE - 1);
    fs->current_path[PATH_SIZE - 1] = '\0';
}

void print_current_path(FileSystem* fs) {
    if (fs == NULL) return;

    update_current_path(fs);
    printf("%s\n", fs->current_path);
}

int is_directory(Node* node) {
    return node != NULL && node->type == NODE_DIR;
}

int is_file(Node* node) {
    return node != NULL && node->type == NODE_FILE;
}

int has_child(Node* node) {
    return node != NULL && node->child != NULL;
}

int is_duplicate_name(Node* parent, const char* name) {
    return find_child(parent, name) != NULL;
}

void update_modified_time(Node* node) {
    if (node == NULL) return;

    node->modified_at = time(NULL);
}

int set_file_content(Node* file, const char* content, int size) {
    if (file == NULL || file->type != NODE_FILE) return -1;
    if (content == NULL || size < 0) return -1;

    char* new_content = (char*)malloc(size + 1);

    if (new_content == NULL) {
        printf("filesystem: content allocation failed\n");
        return -1;
    }

    memcpy(new_content, content, size);
    new_content[size] = '\0';

    if (file->content != NULL) {
        free(file->content);
    }

    file->content = new_content;
    file->size = size;

    update_modified_time(file);

    return 0;
}

void format_permission(Node* node, char* out) {
    if (node == NULL || out == NULL) return;

    int mode = node->permission;

    int user = mode / 100;
    int group = (mode / 10) % 10;
    int other = mode % 10;

    out[0] = (node->type == NODE_DIR) ? 'd' : '-';

    out[1] = (user & 4) ? 'r' : '-';
    out[2] = (user & 2) ? 'w' : '-';
    out[3] = (user & 1) ? 'x' : '-';

    out[4] = (group & 4) ? 'r' : '-';
    out[5] = (group & 2) ? 'w' : '-';
    out[6] = (group & 1) ? 'x' : '-';

    out[7] = (other & 4) ? 'r' : '-';
    out[8] = (other & 2) ? 'w' : '-';
    out[9] = (other & 1) ? 'x' : '-';

    out[10] = '\0';
}