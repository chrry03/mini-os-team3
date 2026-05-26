#include "commands.h"

void command_chown(FileSystem* fs, int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: chown [owner][:[group]] <target>\n");
        return;
    }

    pthread_mutex_lock(&fs->lock);

    // argv[1]은 "u1", "u1.g1", ".g1" 등의 문자열
    // argv[2]는 대상 파일이나 디렉토리
    char* ug_str = argv[1]; 
    char* target_path = argv[2]; 

    Node* target = resolve_path(fs, target_path);
    if (!target) {
        printf("Wrong Directory or File\n"); // 예시 코드와 동일한 에러 메시지
        pthread_mutex_unlock(&fs->lock);
        return;
    }

    // 권한 확인 로직은 일단 제외 (필요시 Check_Permission 관련 로직 추가)

    char owner[OWNER_SIZE] = {0};
    char group[OWNER_SIZE] = {0};

    // 1. ".[group]" 또는 ":[group]" 형태 (그룹만 변경)
    if (ug_str[0] == '.' || ug_str[0] == ':') {
        printf("Change only Group\n\n");
        strncpy(group, ug_str + 1, OWNER_SIZE - 1);
    } 
    // 2. "[owner].[group]" 또는 "[owner]" 형태
    else {
        // 구분자가 있는지 찾음 (':' 또는 '.')
        char* sep = strchr(ug_str, ':');
        if (!sep) sep = strchr(ug_str, '.');

        if (sep != NULL) {
            // 구분자가 있으면 소유자와 그룹 모두 변경
            printf("Change User and Group\n\n");
            *sep = '\0'; // 문자열을 두 개로 쪼갬
            strncpy(owner, ug_str, OWNER_SIZE - 1);
            strncpy(group, sep + 1, OWNER_SIZE - 1);
        } else {
            // 구분자가 없으면 소유자만 변경
            printf("Change only User\n\n");
            strncpy(owner, ug_str, OWNER_SIZE - 1);
        }
    }

    // 3. 분리해낸 문자열을 실제 노드에 적용
    if (strlen(owner) > 0) {
        strncpy(target->owner, owner, OWNER_SIZE - 1);
        target->owner[OWNER_SIZE - 1] = '\0';
    }
    if (strlen(group) > 0) {
        strncpy(target->group, group, OWNER_SIZE - 1);
        target->group[OWNER_SIZE - 1] = '\0';
    }
    
    update_modified_time(target);

    pthread_mutex_unlock(&fs->lock);
}