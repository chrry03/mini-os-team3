#include <stdio.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

void command_touch(FileSystem* fs, int argc, char* argv[]) {
    // 인자가 부족한 경우 예외 처리
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return;
    }

    pthread_mutex_lock(&fs->lock); // 스레드 안전성 확보

    char basename[NAME_SIZE];
    // 상위 디렉토리 노드와 생성/수정할 파일의 이름을 분리
    Node* parent = resolve_parent_path(fs, argv[1], basename);

    // 부모 경로가 유효하지 않으면 에러 출력
    if (!parent) {
        printf("touch: cannot touch '%s': No such file or directory\n", argv[1]);
        pthread_mutex_unlock(&fs->lock);
        return;
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
        }
    }

    pthread_mutex_unlock(&fs->lock); // 락 해제
}