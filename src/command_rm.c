#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

void command_rm(FileSystem* fs, int argc, char* argv[]) {
    if (fs == NULL || argc < 2) {
        printf("Usage: rm [-rf] [file or directory path]\n");
        return;
    }

    int option_r = 0;
    int option_f = 0;
    char* target_path = NULL;

    // 1. 글자 단위 옵션 파싱 (-rf, -r -f, -fr 등 모두 대응)
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'r') {
                    option_r = 1;
                } else if (argv[i][j] == 'f') {
                    option_f = 1;
                } else {
                    printf("rm: invalid option -- '%c'\n", argv[i][j]);
                    return;
                }
            }
        } else {
            // 옵션이 아닌 첫 번째 일반 인자를 타겟 경로로 지정
            if (target_path == NULL) {
                target_path = argv[i];
            }
        }
    }

    if (target_path == NULL) {
        if (option_f != 1) {
            printf("rm: missing operand\n");
        }
        return;
    }

    // 루트 디렉토리 삭제 제한
    if (strcmp(target_path, "/") == 0) {
        printf("rm: refusing to remove root directory '/'\n");
        return;
    }

    pthread_mutex_lock(&fs->lock); //pthread.h 함수, 멀티스레딩에서 충돌 방지 위해 파일 시스템 잠금

    // 2. 대상 노드 탐색
    Node* target = resolve_path(fs, target_path);
    if (target == NULL) {
        // -f 옵션이 있으면 존재하지 않는 파일이어도 에러 메시지 없이 종료
        if (option_f != 1) {
            printf("rm: cannot remove '%s': No such file or directory\n", target_path);
        }
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 루트 디렉토리 삭제 제한
    if (target == fs->root) {
        printf("rm: refusing to remove root directory '/'\n");
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 3. 현재 위치 또는 상위 디렉토리 삭제 방지 검사
    Node* check = fs->current;
    while (check != NULL) {
        if (check == target) {
            printf("rm: cannot remove current working directory or its parent\n");
            pthread_mutex_unlock(&fs->lock);
            return;
        }
        check = check->parent;
    }

    // 4. 디렉토리 노드 타입 체크 (NODE_DIR 상태 직접 비교)
    if (target->type == NODE_DIR && option_r != 1) {
        if (option_f != 1) {
            printf("rm: cannot remove '%s': Is a directory\n", target_path);
        }
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    Node* parent = target->parent;
    if (parent == NULL) {
        if (option_f != 1) {
            printf("rm: cannot find parent node\n");
        }
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 5. 트리 구조 연결 해제 및 메모리 비우기
    remove_child(parent, target);
    free_subtree(target);

    // 6. 상태 최신화 및 락 해제
    update_modified_time(parent);
    update_current_path(fs);
    pthread_mutex_unlock(&fs->lock);
}