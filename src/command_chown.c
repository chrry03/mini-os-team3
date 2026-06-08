#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

#define VALID_USER_COUNT 5
#define VALID_GROUP_COUNT 5

static const char* VALID_USERS[VALID_USER_COUNT] = {
    "root",
    "user1",
    "user2",
    "user3",
    "osmanager"
};

static const char* VALID_GROUPS[VALID_GROUP_COUNT] = {
    "root",
    "user1",
    "user2",
    "user3",
    "osmanager"
};

static int is_valid_user_name(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (int i = 0; i < VALID_USER_COUNT; i++) {
        if (strcmp(name, VALID_USERS[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static int is_valid_group_name(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (int i = 0; i < VALID_GROUP_COUNT; i++) {
        if (strcmp(name, VALID_GROUPS[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

void command_chown(FileSystem* fs, int argc, char* argv[]) {
    if (fs == NULL) {
        printf("chown: filesystem is not initialized\n");
        return;
    }

    if (argc != 3) {
        printf("Usage: chown [owner][:[group]] <target>\n");
        return;
    }

    pthread_mutex_lock(&fs->lock);

    char ug_buffer[OWNER_SIZE * 2];
    char owner[OWNER_SIZE] = {0};
    char group[OWNER_SIZE] = {0};

    strncpy(ug_buffer, argv[1], sizeof(ug_buffer) - 1);
    ug_buffer[sizeof(ug_buffer) - 1] = '\0';

    char* target_path = argv[2];

    Node* target = resolve_path(fs, target_path);

    if (target == NULL) {
        printf("chown: cannot access '%s': No such file or directory\n", target_path);
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    if (ug_buffer[0] == ':' || ug_buffer[0] == '.') {
        strncpy(group, ug_buffer + 1, OWNER_SIZE - 1);
        group[OWNER_SIZE - 1] = '\0';

        if (!is_valid_group_name(group)) {
            printf("chown: invalid group: %s\n", group);
            pthread_mutex_unlock(&fs->lock);
            return;
        }
    } else {
        char* sep = strchr(ug_buffer, ':');

        if (sep == NULL) {
            sep = strchr(ug_buffer, '.');
        }

        if (sep != NULL) {
            *sep = '\0';

            strncpy(owner, ug_buffer, OWNER_SIZE - 1);
            owner[OWNER_SIZE - 1] = '\0';

            strncpy(group, sep + 1, OWNER_SIZE - 1);
            group[OWNER_SIZE - 1] = '\0';

            if (!is_valid_user_name(owner)) {
                printf("chown: invalid user: %s\n", owner);
                pthread_mutex_unlock(&fs->lock);
                return;
            }

            if (!is_valid_group_name(group)) {
                printf("chown: invalid group: %s\n", group);
                pthread_mutex_unlock(&fs->lock);
                return;
            }
        } else {
            strncpy(owner, ug_buffer, OWNER_SIZE - 1);
            owner[OWNER_SIZE - 1] = '\0';

            if (!is_valid_user_name(owner)) {
                printf("chown: invalid user: %s\n", owner);
                pthread_mutex_unlock(&fs->lock);
                return;
            }
        }
    }

    if (owner[0] != '\0') {
        strncpy(target->owner, owner, OWNER_SIZE - 1);
        target->owner[OWNER_SIZE - 1] = '\0';
    }

    if (group[0] != '\0') {
        strncpy(target->group, group, OWNER_SIZE - 1);
        target->group[OWNER_SIZE - 1] = '\0';
    }

    update_modified_time(target);

    pthread_mutex_unlock(&fs->lock);
}