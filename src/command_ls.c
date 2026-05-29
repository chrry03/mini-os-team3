#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

#define COLOR_DIR "\033[1;34m"
#define COLOR_RESET "\033[0m"

static void make_time_string(time_t target_time, char* buffer, int buffer_size)
{
    struct tm* time_info;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }

    time_info = localtime(&target_time);

    if (time_info == NULL)
    {
        strncpy(buffer, "unknown-time", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return;
    }

    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", time_info);
}

static void print_name_with_color(Node* node, const char* display_name)
{
    if (node == NULL)
    {
        return;
    }

    if (display_name == NULL)
    {
        display_name = node->name;
    }

    if (node->type == NODE_DIR)
    {
        printf(COLOR_DIR "%s" COLOR_RESET, display_name);
    }
    else
    {
        printf("%s", display_name);
    }
}

static void print_simple_name(Node* node, const char* display_name)
{
    if (node == NULL)
    {
        return;
    }

    print_name_with_color(node, display_name);
}

static void print_long_info(Node* node, const char* display_name)
{
    char permission[11];
    char time_buffer[64];

    if (node == NULL)
    {
        return;
    }

    if (display_name == NULL)
    {
        display_name = node->name;
    }

    format_permission(node, permission);
    make_time_string(node->modified_at, time_buffer, sizeof(time_buffer));

    printf("%s 1 %-10s %-10s %6d %s ",
           permission,
           node->owner,
           node->group,
           node->size,
           time_buffer);

    print_name_with_color(node, display_name);
    printf("\n");
}

static void print_one_node(Node* node, const char* display_name, int long_format)
{
    if (node == NULL)
    {
        return;
    }

    if (long_format)
    {
        print_long_info(node, display_name);
    }
    else
    {
        print_simple_name(node, display_name);
    }
}

static void print_directory_list(Node* dir, int show_all, int long_format)
{
    Node* cur;
    int printed_count = 0;

    if (dir == NULL || dir->type != NODE_DIR)
    {
        return;
    }

    if (show_all)
    {
        print_one_node(dir, ".", long_format);

        if (!long_format)
        {
            printf("  ");
            printed_count++;
        }

        if (dir->parent != NULL)
        {
            print_one_node(dir->parent, "..", long_format);
        }
        else
        {
            print_one_node(dir, "..", long_format);
        }

        if (!long_format)
        {
            printf("  ");
            printed_count++;
        }
    }

    cur = dir->child;

    while (cur != NULL)
    {
        if (show_all || cur->name[0] != '.')
        {
            print_one_node(cur, cur->name, long_format);

            if (!long_format)
            {
                printf("  ");
                printed_count++;
            }
        }

        cur = cur->sibling;
    }

    if (!long_format && printed_count > 0)
    {
        printf("\n");
    }
}

static int parse_ls_option(const char* option, int* show_all, int* long_format)
{
    int i;

    if (option == NULL || option[0] != '-')
    {
        return 0;
    }

    if (strcmp(option, "--") == 0)
    {
        return 0;
    }

    for (i = 1; option[i] != '\0'; i++)
    {
        if (option[i] == 'a')
        {
            *show_all = 1;
        }
        else if (option[i] == 'l')
        {
            *long_format = 1;
        }
        else
        {
            printf("ls: invalid option -- '%c'\n", option[i]);
            return -1;
        }
    }

    return 1;
}

static void print_ls_target(FileSystem* fs, const char* path, int show_all, int long_format)
{
    Node* target;

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
        printf("ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    if (target->type == NODE_FILE)
    {
        print_one_node(target, target->name, long_format);

        if (!long_format)
        {
            printf("\n");
        }

        return;
    }

    print_directory_list(target, show_all, long_format);
}

void command_ls(FileSystem* fs, int argc, char* argv[])
{
    int show_all = 0;
    int long_format = 0;
    int target_count = 0;
    int i;

    if (fs == NULL)
    {
        printf("ls: filesystem error\n");
        return;
    }

    pthread_mutex_lock(&(fs->lock));

    for (i = 1; i < argc; i++)
    {
        int option_result = parse_ls_option(argv[i], &show_all, &long_format);

        if (option_result == -1)
        {
            pthread_mutex_unlock(&(fs->lock));
            return;
        }

        if (option_result == 0)
        {
            target_count++;
        }
    }

    if (target_count == 0)
    {
        print_ls_target(fs, NULL, show_all, long_format);
        pthread_mutex_unlock(&(fs->lock));
        return;
    }

    for (i = 1; i < argc; i++)
    {
        int option_result = parse_ls_option(argv[i], &show_all, &long_format);

        if (option_result == 0)
        {
            if (target_count > 1)
            {
                printf("%s:\n", argv[i]);
            }

            print_ls_target(fs, argv[i], show_all, long_format);

            if (target_count > 1)
            {
                printf("\n");
            }
        }
    }

    pthread_mutex_unlock(&(fs->lock));
}