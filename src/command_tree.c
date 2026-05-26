#include <stdio.h>
#include <string.h>
#include "filesystem.h"
#include "commands.h"

static int parse_tree_option(const char* option, int* show_all) 
{
    int i;

    if (option == NULL || option[0] != '-') 
    {
        return 0;
    }

    for (i = 1; option[i] != '\0'; i++) 
    {
        if (option[i] == 'a') {
            *show_all = 1;
        } 
        else 
        {
            printf("tree: invalid option -- '%c'\n", option[i]);
            return -1;
        }
    }

    return 1;
}

static int should_print(Node* node, int show_all)
 {
    if (node == NULL) 
    {
        return 0;
    }

    if (show_all) 
    {
        return 1;
    }

    if (node->name[0] == '.') 
    {
        return 0;
    }

    return 1;
}

static int has_next_printable(Node* node, int show_all) 
{
    Node* cur;

    if (node == NULL)
     {
        return 0;
    }

    cur = node->sibling;

    while (cur != NULL) 
    {
        if (should_print(cur, show_all)) 
        {
            return 1;
        }

        cur = cur->sibling;
    }

    return 0;
}

static void print_tree_recursive(Node* node,const char* prefix, int is_last, int show_all, int* dir_count, int* file_count)
{
    Node* child;
    char next_prefix[PATH_SIZE];

    if (node == NULL) 
    {
        return;
    }

    printf("%s", prefix);

    if (is_last) 
    {
        printf("`-- ");
    } else {
        printf("|-- ");
    }

    printf("%s", node->name);

    if (node->type == NODE_DIR) 
    {
        printf("/");
        (*dir_count)++;
    } else {
        (*file_count)++;
    }

    printf("\n");

    if (node->type != NODE_DIR) 
    {
        return;
    }

    if (is_last) 
    {
        snprintf(next_prefix, sizeof(next_prefix), "%s    ", prefix);
    } else 
    {
        snprintf(next_prefix, sizeof(next_prefix), "%s|   ", prefix);
    }

    child = node->child;

    while (child != NULL) 
    {
        if (should_print(child, show_all)) 
        {
            print_tree_recursive(child,next_prefix,!has_next_printable(child, show_all),show_all,dir_count, file_count);
        }

        child = child->sibling;
    }
}

static void print_tree(FileSystem* fs,const char* path,int show_all) 
{
    Node* target;
    Node* child;
    int dir_count = 0;
    int file_count = 0;

    if (fs == NULL) 
    {
        return;
    }

    if (path == NULL) 
    {
        target = fs->current;
    } 
    else 
    {
        target = resolve_path(fs, path);
    }

    if (target == NULL) 
    {
        printf("tree: cannot access '%s': No such file or directory\n", path);
        return;
    }

    if (target->type == NODE_FILE)
     {
        printf("%s\n", target->name);
        printf("\n0 directories, 1 file\n");
        return;
    }

    if (target == fs->root) 
    {
        printf("/\n");
    } else {
        printf("%s/\n", target->name);
    }

    child = target->child;

    while (child != NULL) 
    {
        if (should_print(child, show_all)) 
        {
            print_tree_recursive(child, "", !has_next_printable(child, show_all), show_all, &dir_count, &file_count);
        }

        child = child->sibling;
    }

    printf("\n%d directories, %d files\n",dir_count,file_count);
}

void command_tree(FileSystem* fs, int argc, char* argv[]) 
{
    int show_all = 0;
    int i;
    int printed = 0;

    if (fs == NULL) 
    {
        printf("tree: filesystem error\n");
        return;
    }

    pthread_mutex_lock(&(fs->lock));

    for (i = 1; i < argc; i++) 
    {
        int result;

        result = parse_tree_option(argv[i], &show_all);

        if (result == -1) 
        {
            pthread_mutex_unlock(&(fs->lock));
            return;
        }
    }

    for (i = 1; i < argc; i++) 
    {
        if (argv[i][0] != '-') 
        {
            print_tree(fs, argv[i], show_all);
            printed = 1;

            if (i < argc - 1) 
            {
                printf("\n");
            }
        }
    }

    if (!printed) 
    {
        print_tree(fs, NULL, show_all);
    }

    pthread_mutex_unlock(&(fs->lock));
}