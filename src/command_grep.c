#include "commands.h"
#include <ctype.h>

// 대소문자 무시를 위한 문자열 소문자 변환 헬퍼 함수
static void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

void command_grep(FileSystem* fs, int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: grep [-nvi] <pattern> <file>\n");
        return;
    }

    int show_line = 0;
    int invert_match = 0;
    int ignore_case = 0;
    
    int opt_idx = 1;

    // 1. 옵션 파싱 로직 (-n, -v, -i 조합 처리)
    while (opt_idx < argc && argv[opt_idx][0] == '-') {
        for (int i = 1; argv[opt_idx][i] != '\0'; i++) {
            if (argv[opt_idx][i] == 'n') {
                show_line = 1;
            } else if (argv[opt_idx][i] == 'v') {
                invert_match = 1;
            } else if (argv[opt_idx][i] == 'i') {
                ignore_case = 1;
            } else {
                printf("grep: invalid option -- '%c'\n", argv[opt_idx][i]);
                return;
            }
        }
        opt_idx++;
    }

    // 옵션을 제외하고 남은 인자가 패턴과 파일명 2개인지 확인
    if (argc - opt_idx < 2) {
        printf("Usage: grep [-nvi] <pattern> <file>\n");
        return;
    }

    char* pattern = argv[opt_idx];
    char* filename = argv[opt_idx + 1];

    pthread_mutex_lock(&fs->lock);

    Node* target = resolve_path(fs, filename);

    if (!target || !is_file(target)) {
        printf("grep: %s: No such file\n", filename);
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 2. 내용 탐색 및 옵션 적용 (권한 확인 생략됨)
    if (target->content != NULL) {
        char* content_copy = strdup(target->content);
        if (content_copy) {
            char* line = strtok(content_copy, "\n");
            int line_count = 1;

            // -i 옵션을 위한 패턴 소문자 복사본 생성
            char lower_pattern[256];
            if (ignore_case) {
                strncpy(lower_pattern, pattern, sizeof(lower_pattern) - 1);
                lower_pattern[sizeof(lower_pattern) - 1] = '\0';
                to_lowercase(lower_pattern);
            }

            while (line != NULL) {
                int match = 0;
                
                // -i 옵션이 켜져있으면 임시로 줄 내용을 소문자로 바꿔서 비교
                if (ignore_case) {
                    char temp_line[1024];
                    strncpy(temp_line, line, sizeof(temp_line) - 1);
                    temp_line[sizeof(temp_line) - 1] = '\0';
                    to_lowercase(temp_line);
                    
                    if (strstr(temp_line, lower_pattern) != NULL) {
                        match = 1;
                    }
                } else {
                    // 기본 검색 (대소문자 구분)
                    if (strstr(line, pattern) != NULL) {
                        match = 1;
                    }
                }

                // -v (invert) 로직 적용하여 출력 여부 결정
                if ((match && !invert_match) || (!match && invert_match)) {
                    // -n (line number) 로직 적용
                    if (show_line) {
                        printf("%d:", line_count);
                    }
                    printf("%s\n", line);
                }

                line = strtok(NULL, "\n");
                line_count++;
            }
            free(content_copy);
        }
    }

    pthread_mutex_unlock(&fs->lock);
}