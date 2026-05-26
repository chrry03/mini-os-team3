#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "filesystem.h"
#include "commands.h"

void command_mv(FileSystem* fs, int argc, char* argv[]) {
    if (fs == NULL || argc < 3) {
        printf("Usage: mv [source path] [destination path]\n");
        return;
    }

    char* src_path = NULL;
    char* dest_path = NULL;

    // 1. 인자 파싱 (-로 시작하는 옵션이 들어와도 유연하게 처리하고 경로만 추출)
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            continue;
        } else {
            if (src_path == NULL) {
                src_path = argv[i];
            } else if (dest_path == NULL) {
                dest_path = argv[i];
            }
        }
    }

    // 출발지와 목적지 경로가 모두 지정되지 않은 경우 예외 처리
    if (src_path == NULL || dest_path == NULL) {
        printf("mv: missing destination file operand after '%s'\n", src_path ? src_path : "source");
        return;
    }

    pthread_mutex_lock(&fs->lock);  //pthread.h 함수, 멀티스레딩에서 충돌 방지 위해 파일 시스템 잠금

    // 2. 출발지 노드 탐색 및 검증
    Node* src_node = resolve_path(fs, src_path);
    if (src_node == NULL) {
        printf("mv: cannot stat '%s': No such file or directory\n", src_path);
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 루트 디렉토리 이동 제한
    if (src_node == fs->root) {
        printf("mv: cannot move root directory\n");
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 3. 목적지 경로 해석 및 새 부모(target_parent) 설정
    Node* dest_node = resolve_path(fs, dest_path);
    Node* target_parent = NULL;
    char new_name[NAME_SIZE] = {0};

    if (dest_node != NULL) {
        // 경우 A: 목적지가 이미 존재하는 디렉토리인 경우 -> 그 내부로 원본 노드를 이동
        if (dest_node->type == NODE_DIR) {
            target_parent = dest_node;
            strncpy(new_name, src_node->name, NAME_SIZE - 1);
        } else {
            printf("mv: '%s' already exists (overwrite is not supported)\n", dest_path);
            pthread_mutex_unlock(&fs->lock);
            return;
        }
    } else {
        // 경우 B: 목적지가 존재하지 않는 경로인 경우 -> 이름 변경(Rename) 또는 새 경로로 이동
        target_parent = resolve_parent_path(fs, dest_path, new_name);
        if (target_parent == NULL || target_parent->type != NODE_DIR) {
            printf("mv: cannot move to '%s': Invalid destination path\n", dest_path);
            pthread_mutex_unlock(&fs->lock);
            return;
        }
    }

    // 4. 중복 이름 체크
    if (is_duplicate_name(target_parent, new_name)) {
        printf("mv: cannot move: '%s' already exists in '%s'\n", new_name, target_parent->name);
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 5. 상위 폴더를 자기 하위 폴더로 이동시키는 트리 꼬임 오류 검사
    Node* check = target_parent;
    while (check != NULL) {
        if (check == src_node) {
            printf("mv: cannot move a directory into itself or its subdirectory\n");
            pthread_mutex_unlock(&fs->lock);
            return;
        }
        check = check->parent;
    }

    // 6. 기존 부모 연결 해제 및 새 부모 노드에 등록 (트리 구조 갱신)
    remove_child(src_node->parent, src_node);
    
    strncpy(src_node->name, new_name, NAME_SIZE - 1);
    add_child(target_parent, src_node);
    
    // 7. 시간 설정 및 가상 파일 시스템 경로 최신화
    update_modified_time(src_node);
    update_modified_time(target_parent);
    update_current_path(fs);

    pthread_mutex_unlock(&fs->lock);
}