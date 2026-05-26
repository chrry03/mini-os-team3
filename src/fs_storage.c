#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_storage.h"

#define STORAGE_MAX_LINE 16384
#define STORAGE_MAX_DEPTH 256

static char hex_digit(int value) {
    if (value >= 0 && value <= 9) {
        return (char)('0' + value);
    }

    return (char)('A' + (value - 10));
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return -1;
}

static char* encode_hex(const unsigned char* data, int size) {
    if (data == NULL || size <= 0) {
        char* empty = (char*)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }

    char* encoded = (char*)malloc((size * 2) + 1);

    if (encoded == NULL) {
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        encoded[i * 2] = hex_digit((data[i] >> 4) & 0x0F);
        encoded[i * 2 + 1] = hex_digit(data[i] & 0x0F);
    }

    encoded[size * 2] = '\0';

    return encoded;
}

static unsigned char* decode_hex(const char* text, int* out_size) {
    if (out_size != NULL) {
        *out_size = 0;
    }

    if (text == NULL || text[0] == '\0') {
        unsigned char* empty = (unsigned char*)malloc(1);
        if (empty != NULL) {
            empty[0] = '\0';
        }
        return empty;
    }

    int length = (int)strlen(text);

    if (length % 2 != 0) {
        return NULL;
    }

    int size = length / 2;
    unsigned char* decoded = (unsigned char*)malloc(size + 1);

    if (decoded == NULL) {
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        int high = hex_value(text[i * 2]);
        int low = hex_value(text[i * 2 + 1]);

        if (high < 0 || low < 0) {
            free(decoded);
            return NULL;
        }

        decoded[i] = (unsigned char)((high << 4) | low);
    }

    decoded[size] = '\0';

    if (out_size != NULL) {
        *out_size = size;
    }

    return decoded;
}

static void attach_child_without_time_update(Node* parent, Node* child) {
    if (parent == NULL || child == NULL) return;
    if (parent->type != NODE_DIR) return;

    child->parent = parent;
    child->sibling = NULL;

    if (parent->child == NULL) {
        parent->child = child;
        return;
    }

    Node* cur = parent->child;

    while (cur->sibling != NULL) {
        cur = cur->sibling;
    }

    cur->sibling = child;
}

static int save_node(FILE* file, Node* node, int depth) {
    if (file == NULL || node == NULL) {
        return -1;
    }

    char* encoded_name = encode_hex((const unsigned char*)node->name, (int)strlen(node->name));
    char* encoded_owner = encode_hex((const unsigned char*)node->owner, (int)strlen(node->owner));
    char* encoded_group = encode_hex((const unsigned char*)node->group, (int)strlen(node->group));

    char* encoded_content = NULL;

    if (node->type == NODE_FILE && node->content != NULL && node->size > 0) {
        encoded_content = encode_hex((const unsigned char*)node->content, node->size);
    } else {
        encoded_content = encode_hex(NULL, 0);
    }

    if (encoded_name == NULL || encoded_owner == NULL || encoded_group == NULL || encoded_content == NULL) {
        free(encoded_name);
        free(encoded_owner);
        free(encoded_group);
        free(encoded_content);
        return -1;
    }

    fprintf(file,
            "%d|%d|%s|%s|%s|%d|%d|%ld|%ld|%s\n",
            depth,
            node->type,
            encoded_name,
            encoded_owner,
            encoded_group,
            node->permission,
            node->size,
            (long)node->created_at,
            (long)node->modified_at,
            encoded_content);

    free(encoded_name);
    free(encoded_owner);
    free(encoded_group);
    free(encoded_content);

    Node* child = node->child;

    while (child != NULL) {
        if (save_node(file, child, depth + 1) != 0) {
            return -1;
        }

        child = child->sibling;
    }

    return 0;
}

int save_filesystem(FileSystem* fs, const char* filename) {
    if (fs == NULL || fs->root == NULL || filename == NULL) {
        return -1;
    }

    FILE* file = fopen(filename, "w");

    if (file == NULL) {
        printf("storage: failed to open file for saving: %s\n", filename);
        return -1;
    }

    int result = save_node(file, fs->root, 0);

    fclose(file);

    if (result != 0) {
        printf("storage: failed to save filesystem\n");
        return -1;
    }

    return 0;
}

static int restore_string_field(char* destination, int destination_size, const char* encoded_text) {
    if (destination == NULL || destination_size <= 0 || encoded_text == NULL) {
        return -1;
    }

    int decoded_size = 0;
    unsigned char* decoded = decode_hex(encoded_text, &decoded_size);

    if (decoded == NULL) {
        return -1;
    }

    int copy_size = decoded_size;

    if (copy_size >= destination_size) {
        copy_size = destination_size - 1;
    }

    memcpy(destination, decoded, copy_size);
    destination[copy_size] = '\0';

    free(decoded);

    return 0;
}

int load_filesystem(FileSystem* fs, const char* filename) {
    if (fs == NULL || filename == NULL) {
        return -1;
    }

    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        if (fs->root == NULL) {
            fs->root = create_node("/", NODE_DIR);
            fs->current = fs->root;
            update_current_path(fs);
        }

        return 0;
    }

    if (fs->root != NULL) {
        free_subtree(fs->root);
        fs->root = NULL;
        fs->current = NULL;
        fs->current_path[0] = '\0';
    }

    Node* depth_stack[STORAGE_MAX_DEPTH];

    for (int i = 0; i < STORAGE_MAX_DEPTH; i++) {
        depth_stack[i] = NULL;
    }

    char line[STORAGE_MAX_LINE];

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        char* saveptr = NULL;

        char* depth_text = strtok_r(line, "|", &saveptr);
        char* type_text = strtok_r(NULL, "|", &saveptr);
        char* name_text = strtok_r(NULL, "|", &saveptr);
        char* owner_text = strtok_r(NULL, "|", &saveptr);
        char* group_text = strtok_r(NULL, "|", &saveptr);
        char* permission_text = strtok_r(NULL, "|", &saveptr);
        char* size_text = strtok_r(NULL, "|", &saveptr);
        char* created_text = strtok_r(NULL, "|", &saveptr);
        char* modified_text = strtok_r(NULL, "|", &saveptr);
        char* content_text = strtok_r(NULL, "|", &saveptr);

        if (depth_text == NULL || type_text == NULL || name_text == NULL ||
            owner_text == NULL || group_text == NULL || permission_text == NULL ||
            size_text == NULL || created_text == NULL || modified_text == NULL) {
            continue;
        }

        int depth = atoi(depth_text);

        if (depth < 0 || depth >= STORAGE_MAX_DEPTH) {
            continue;
        }

        NodeType type = (NodeType)atoi(type_text);

        if (type != NODE_FILE && type != NODE_DIR) {
            continue;
        }

        char name[NAME_SIZE];

        if (restore_string_field(name, NAME_SIZE, name_text) != 0) {
            continue;
        }

        Node* node = create_node(name, type);

        if (node == NULL) {
            continue;
        }

        restore_string_field(node->owner, OWNER_SIZE, owner_text);
        restore_string_field(node->group, OWNER_SIZE, group_text);

        node->permission = atoi(permission_text);
        node->size = atoi(size_text);
        node->created_at = (time_t)atol(created_text);
        node->modified_at = (time_t)atol(modified_text);

        if (type == NODE_FILE) {
            int decoded_content_size = 0;
            unsigned char* decoded_content = NULL;

            if (content_text != NULL) {
                decoded_content = decode_hex(content_text, &decoded_content_size);
            } else {
                decoded_content = decode_hex("", &decoded_content_size);
            }

            if (decoded_content != NULL) {
                time_t saved_created_at = node->created_at;
                time_t saved_modified_at = node->modified_at;

                set_file_content(node, (const char*)decoded_content, decoded_content_size);

                node->created_at = saved_created_at;
                node->modified_at = saved_modified_at;

                free(decoded_content);
            }
        }

        if (depth == 0) {
            fs->root = node;
        } else {
            Node* parent = depth_stack[depth - 1];

            if (parent != NULL) {
                attach_child_without_time_update(parent, node);
            } else {
                free_subtree(node);
                continue;
            }
        }

        depth_stack[depth] = node;

        for (int i = depth + 1; i < STORAGE_MAX_DEPTH; i++) {
            depth_stack[i] = NULL;
        }
    }

    fclose(file);

    if (fs->root == NULL) {
        fs->root = create_node("/", NODE_DIR);
    }

    fs->current = fs->root;
    update_current_path(fs);

    return 0;
} //depth, type, name, owner, group, permission, size, created_at, modified_at, content 저장