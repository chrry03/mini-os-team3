#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

static void* touch_thread_func(void* arg) {
    ThreadArg* thread_arg = (ThreadArg*)arg;

    if (thread_arg == NULL || thread_arg->fs == NULL) {
        return NULL;
    }

    FileSystem* fs = thread_arg->fs;
    char basename[NAME_SIZE];

    pthread_mutex_lock(&fs->lock); // 스레드 안전성 확보

    // 상위 디렉토리 노드와 생성/수정할 파일의 이름을 분리
    Node* parent = resolve_parent_path(fs, thread_arg->target_path, basename);

    // 부모 경로가 유효하지 않으면 에러 출력
    if (!parent) {
        printf("touch: cannot touch '%s': No such file or directory\n", thread_arg->target_path);
        pthread_mutex_unlock(&fs->lock);
        return NULL;
    }

    if (parent->type != NODE_DIR) {
        printf("touch: cannot touch '%s': Not a directory\n", thread_arg->target_path);
        pthread_mutex_unlock(&fs->lock);
        return NULL;
    }

    // 부모 디렉토리 안에 동일한 이름의 자식 노드가 있는지 탐색
    Node* target = find_child(parent, basename);

    if (target) {
        // 1. 이미 존재하면 최종 수정 시간만 업데이트
        update_modified_time(target);
    } else {
        // 2. 존재하지 않으면 새 파일 노드 생성 및 연결
        Node* new_file = create_node(basename, NODE_FILE);
        if (new_file) {
            add_child(parent, new_file); // 부모-자식 트리 구조로 연결
        } else {
            printf("touch: failed to create '%s'\n", thread_arg->target_path);
        }
    }

    pthread_mutex_unlock(&fs->lock); // 락 해제

    return NULL;
}

void command_touch(FileSystem* fs, int argc, char* argv[]) {
    // 인자가 부족한 경우 예외 처리
    if (argc < 2) {
        printf("Usage: touch <filename> [filename...]\n");
        return;
    }

    if (fs == NULL) {
        printf("touch: filesystem is not initialized\n");
        return;
    }

    int target_count = argc - 1;

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * target_count);
    ThreadArg* thread_args = (ThreadArg*)malloc(sizeof(ThreadArg) * target_count);
    int* thread_created = (int*)malloc(sizeof(int) * target_count);

    if (threads == NULL || thread_args == NULL || thread_created == NULL) {
        printf("touch: thread allocation failed\n");

        free(threads);
        free(thread_args);
        free(thread_created);

        return;
    }

    // 여러 개의 파일명이 들어온 경우 각각 스레드로 생성/수정 처리
    for (int i = 0; i < target_count; i++) {
        thread_created[i] = 0;

        thread_args[i].fs = fs;
        strncpy(thread_args[i].target_path, argv[i + 1], PATH_SIZE - 1);
        thread_args[i].target_path[PATH_SIZE - 1] = '\0';

        thread_args[i].target_name[0] = '\0';
        thread_args[i].option_flag = 0;

        if (pthread_create(&threads[i], NULL, touch_thread_func, &thread_args[i]) != 0) {
            printf("touch: failed to create thread for '%s'\n", argv[i + 1]);

            // 스레드 생성에 실패한 경우 해당 파일은 현재 스레드에서 직접 처리
            touch_thread_func(&thread_args[i]);
        } else {
            thread_created[i] = 1;
        }
    }

    // 생성된 모든 스레드가 끝날 때까지 대기
    for (int i = 0; i < target_count; i++) {
        if (thread_created[i]) {
            pthread_join(threads[i], NULL);
        }
    }

    free(threads);
    free(thread_args);
    free(thread_created);
}