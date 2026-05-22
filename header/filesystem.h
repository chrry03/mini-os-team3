#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h> //멀티스레딩 관련

#define NAME_SIZE 128
#define PATH_SIZE 1024
#define OWNER_SIZE 64

typedef enum {
    NODE_FILE = 0,
    NODE_DIR = 1
} NodeType; //노드가 파일인지 폴더인지 구분 NodeType

/* 가상 파일 시스템 안의, 파일 또는 폴더 하나를 나타내는 노드 Node */
typedef struct Node {
    char name[NAME_SIZE];
    NodeType type;

    char* content;
    int size;

    char owner[OWNER_SIZE];
    char group[OWNER_SIZE];
    int permission; //숫자로 권한 표현 777 644 등

    time_t created_at;
    time_t modified_at;

    struct Node* parent; //부모, 자식, 형제 노드 가리키는 포인터(링크트리구조)
    struct Node* child;
    struct Node* sibling;
} Node;

/* 전체 파일 시스템의 상태를 관리하는 구조체(FileSystem fs; 이런식으로 정의해서 command함수의 인자로 전달) */
typedef struct FileSystem {
    Node* root; //루트 디렉토리(/)를 가리킴
    Node* current; //현재 위치한 디렉토리 가리킴
    char current_path[PATH_SIZE]; //현재 경로를 문자열로 저장
    pthread_mutex_t lock; //멀티 스레딩에서 충돌을 막기 위함(그부분에서 이걸 갖다가 사용하면 됨)
} FileSystem;

/* 멀티스레딩 작업 시 스레드에 넘겨줄 인자 구조체 */
typedef struct ThreadArg {
    FileSystem* fs;
    char target_name[NAME_SIZE]; //작업 대상 이름
    char target_path[PATH_SIZE]; //작업 대상 경로
    int option_flag; //옵션값
} ThreadArg;

/* 파일 시스템 초기화 및 해제 */
void init_filesystem(FileSystem* fs); //파일시스템 처음 시작할때 호출 main.c에서 사용
void destroy_filesystem(FileSystem* fs); //프로그램 종료시 메모리 정리

/* 노드 생성, 연결, 탐색, 삭제 관련 함수 */
Node* create_node(const char* name, NodeType type); //새파일, 폴더 노드를 만드는 함수
void add_child(Node* parent, Node* child); //부모 폴더 안에 자식 노드 추가
Node* find_child(Node* parent, const char* name); //부모 폴더 안에서 특정이름의 자식을 찾는 함수
void remove_child(Node* parent, Node* target); //부모 폴더에서 특정 자식 노드를 연결 해제하는 함수
void free_subtree(Node* node); //어떤 노드와 그 아래 자식들을 전부 메모리 해제하는 함수(rm -rf 등)

/* 경로 문자열을 실제 노드로 변환하는 함수 */
Node* resolve_path(FileSystem* fs, const char* path); //경로 문자열을 실제 노드로 찾아주는 함수
Node* resolve_parent_path(FileSystem* fs, const char* path, char* basename); //경로에서 부모 디렉토리와 마지막 이름을 분리하는 함수

/* 현재 경로 관리 함수 */
void update_current_path(FileSystem* fs); //현재 위치 fs->current를 기준으로 fs->current_path 문자열을 갱신하는 함수
void print_current_path(FileSystem* fs); //현재 경로를 출력하는 함수

/* 노드 타입 및 상태 확인 함수 */
int is_directory(Node* node); //노드가 디렉토리인지 확인(디렉토리면 1, 아니면 0)
int is_file(Node* node); //노드가 파일인지 확인(파일이면 1, 아니면 0)
int has_child(Node* node); //노드가 자식을 가지고 있는지 확인(빈 디렉토리인지 확인)
int is_duplicate_name(Node* parent, const char* name); //부모 폴더 안에 같은 이름의 파일,폴더가 이미 있는지 확인

/* 파일 내용, 수정 시간, 권한 문자열 관련 함수 */
void update_modified_time(Node* node); //노드의 수정 시간을 현재 시간으로 바꿔줌
int set_file_content(Node* file, const char* content, int size); //파일 노드에 내용을 저장하는 함수
void format_permission(Node* node, char* out); //권한값을 문자열로 바꿔주는 함수(뭐 744이런걸 rwxr--r-- 이런걸로 바꿔줌 ls담당이 쓰면됨)

#endif