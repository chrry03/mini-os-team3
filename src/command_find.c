#include "commands.h"

// 1. 따옴표를 제거해주는 헬퍼 함수 ("*dong*" -> *dong*)
static void strip_quotes(char* str) {
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// 2. 와일드카드(*) 패턴 매칭 헬퍼 함수
static int match_pattern(const char* name, const char* pattern) {
    // 패턴에 '*'가 없으면 기존처럼 완전 일치 검사 (예: "dong")
    if (strchr(pattern, '*') == NULL) {
        return strcmp(name, pattern) == 0;
    }

    // 양 끝에 '*'가 있는 경우 (예: *dong*) -> 부분 일치 검사
    if (pattern[0] == '*' && pattern[strlen(pattern) - 1] == '*') {
        char temp_pattern[NAME_SIZE] = {0};
        strncpy(temp_pattern, pattern + 1, strlen(pattern) - 2);
        return strstr(name, temp_pattern) != NULL;
    }

    // 앞에만 '*'가 있는 경우 (예: *.txt) -> 끝부분(확장자 등) 검사
    if (pattern[0] == '*') {
        const char* suffix = pattern + 1;
        int name_len = strlen(name);
        int suffix_len = strlen(suffix);
        if (name_len >= suffix_len) {
            return strcmp(name + name_len - suffix_len, suffix) == 0;
        }
        return 0;
    }

    // 뒤에만 '*'가 있는 경우 (예: dong*) -> 시작부분 검사
    if (pattern[strlen(pattern) - 1] == '*') {
        return strncmp(name, pattern, strlen(pattern) - 1) == 0;
    }

    return 0;
}

// 디렉토리 재귀 탐색 헬퍼 함수
static void find_recursive(Node* current_node, const char* current_path, const char* target_name, int target_type) {
    if (!current_node) return;

    int name_match = 1;
    int type_match = 1;

    // -name 조건 검사 (와일드카드 패턴 매칭 적용)
    if (target_name != NULL) {
        if (!match_pattern(current_node->name, target_name)) {
            name_match = 0;
        }
    }

    // -type 조건 검사
    if (target_type != -1) {
        if (current_node->type != target_type) {
            type_match = 0;
        }
    }

    // 조건이 모두 참이면 경로 출력
    if (name_match && type_match) {
        printf("%s\n", current_path);
    }

    // 자식 노드 순회
    if (is_directory(current_node)) {
        Node* child = current_node->child;
        while (child != NULL) {
            char next_path[PATH_SIZE];
            
            if (strcmp(current_path, "/") == 0) {
                snprintf(next_path, PATH_SIZE, "/%s", child->name);
            } else {
                snprintf(next_path, PATH_SIZE, "%s/%s", current_path, child->name);
            }
            
            find_recursive(child, next_path, target_name, target_type);
            child = child->sibling;
        }
    }
}

void command_find(FileSystem* fs, int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: find <path> [-name <filename>] [-type f|d]\n");
        return;
    }

    pthread_mutex_lock(&fs->lock);

    char* start_path = argv[1];
    char* target_name = NULL;
    int target_type = -1;

    int i = 2;
    while (i < argc) {
        if (strcmp(argv[i], "-name") == 0 && i + 1 < argc) {
            target_name = argv[i + 1];
            strip_quotes(target_name); // 입력된 따옴표 제거 로직 추가
            i += 2; 
        } 
        else if (strcmp(argv[i], "-type") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "f") == 0) {
                target_type = NODE_FILE;
            } 
            else if (strcmp(argv[i + 1], "d") == 0) {
                target_type = NODE_DIR;
            } 
            else {
                printf("find: unknown argument to -type: %s\n", argv[i + 1]);
                pthread_mutex_unlock(&fs->lock);
                return;
            }
            i += 2;
        } 
        else {
            printf("find: paths must precede expression: %s\n", argv[i]);
            pthread_mutex_unlock(&fs->lock);
            return;
        }
    }

    Node* start_node = resolve_path(fs, start_path);

    if (!start_node) {
        printf("find: '%s': No such file or directory\n", start_path);
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    find_recursive(start_node, start_path, target_name, target_type);

    pthread_mutex_unlock(&fs->lock);
}