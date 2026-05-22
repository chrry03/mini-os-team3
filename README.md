# Mini OS Team 3

운영체제 1분반 프로젝트 Mini OS 구현을 위한 3조 GitHub 레포입니다.

본 프로젝트는 실제 리눅스 파일 시스템을 직접 조작하는 것이 아니라, C언어로 가상 파일 시스템 구조를 만들고 그 위에서 리눅스 기본 명령어처럼 동작하는 시뮬레이터를 구현하는 것을 목표로 합니다.

## 프로젝트 구조

```text
mini-os-team3/
├── Makefile
├── src/
│   ├── main.c
│   ├── filesystem.c
│   ├── fs_storage.c
│   ├── command_ls.c
│   ├── command_cd.c
│   ├── command_pwd.c
│   ├── command_mkdir.c
│   ├── command_cat.c
│   ├── command_grep.c
│   ├── command_chown.c
│   ├── command_mv.c
│   └── command_rm.c
├── header/
│   ├── filesystem.h
│   ├── fs_storage.h
│   └── commands.h
└── data/
    └── filesystem.dat
```

## 파일 역할

| 파일 | 역할 |
|---|---|
| `header/filesystem.h` | Node 구조체, FileSystem 구조체, 공통 함수 선언 |
| `src/filesystem.c` | 노드 생성, 추가, 탐색, 삭제, 경로 처리 등 공통 함수 구현 |
| `header/fs_storage.h` | 파일 시스템 저장/불러오기 함수 선언 |
| `src/fs_storage.c` | `data/filesystem.dat`에 가상 파일 시스템 상태 저장/복원 |
| `header/commands.h` | 각 명령어 함수 선언 |
| `src/command_*.c` | 각 명령어 실제 구현 |
| `src/main.c` | 사용자 입력 처리, 명령어 파싱, 명령어 실행 |
| `Makefile` | 전체 프로젝트 컴파일 |

## 역할분담

- **오채령**: 공통 트리 구조 설계, `main.c`, `Makefile`, `filesystem.h`, `filesystem.c`, `commands.h`, `fs_storage.h`, `fs_storage.c`, Azure VM 최종 배포, 코드 최종 통합, `command_pwd.c` 구현, 보고서 작성
- **서예찬**: `command_ls.c`, `command_cd.c`, 추가 명령어 1개 구현, 보고서 작성
- **노윤서**: `command_mkdir.c`, `command_cat.c` 구현, PPT 제작
- **조용준**: `command_grep.c`, `command_chown.c` 구현, 보고서 작성
- **정인하**: `command_mv.c`, `command_rm.c` 구현, 보고서 작성

## 개발 규칙

1. 최종 실행 기준은 **Ubuntu gcc + make**입니다.
2. 각자 개발 환경은 자유롭게 사용해도 되지만(vscode, visual studio 등), 최종 테스트는 Ubuntu VM에서 진행합니다.
3. Node 구조체와 FileSystem 구조체는 `header/filesystem.h`에 정의된 것을 사용합니다.
4. 각자 `Node` 구조체를 새로 정의하지 않습니다.
5. 각 `command_*.c` 파일에는 아래 include를 넣어 사용해주세요.

```c
#include "filesystem.h"
#include "commands.h"
```

6. 함수 형식은 `header/commands.h`에 선언된 형식을 따릅니다.(함수명, 형식 꼭 맞춰주세요!!!!!! 인자는 만약 저거 바탕으로 더 추가해야하면 하시면 됩니다.)

예시:

```c
void command_ls(FileSystem* fs, int argc, char* argv[]);
void command_cd(FileSystem* fs, int argc, char* argv[]);
```

7. Visual Studio 전용 함수는 사용하지 않습니다.(주의!!!!!)

```c
strcpy_s
scanf_s
```

위와 같은 함수 대신 Ubuntu gcc에서 컴파일 가능한 표준 C 함수를 사용해주세요.

8. 파일명 대소문자를 정확히 맞춥니다.

예시:

```c
#include "filesystem.h"
```

이렇게 작성했다면 실제 파일명도 반드시 `filesystem.h`여야 합니다.

9. **제가 팀원들께 zip파일로 받아서 제 로컬에서 합쳐서 깃허브에 올리고, 깃허브에 최종합쳐진 코드를 mobaXterm으로 vm으로 옮겨서 테스트해보며 개발 완료할 예정입니다.**<br>
한번에 최종코드 올리는게 아니라 계속 중간중간 vm으로 올리면서 테스트 해보며 개발할 예정이에요.

## 명령어 함수 작성 방식

각 명령어는 `src/command_명령어.c` 파일에 작성합니다.

예시: `src/command_ls.c`

```c
#include <stdio.h>
#include <string.h>
#include "filesystem.h"
#include "commands.h"

void command_ls(FileSystem* fs, int argc, char* argv[]) {
    // ls 구현
}
```

## argc, argv 의미

사용자가 입력한 명령어를 공백 기준으로 나눈 값입니다.<br>
argc = argument count : 인자의 개수<br>
argv = argument vector : 인자들의 배열

예시 1:

```bash
ls -al
```

```text
argc = 2
argv[0] = "ls"
argv[1] = "-al"
```

예시 2:

```bash
mkdir test
```

```text
argc = 2
argv[0] = "mkdir"
argv[1] = "test"
```

예시 3:

```bash
rm -rf dir1
```

```text
argc = 3
argv[0] = "rm"
argv[1] = "-rf"
argv[2] = "dir1"
```

## GitHub 사용 방식

GitHub 사용이 가능한 경우:

```bash
git clone <repository-url>
cd mini-os-team3
git checkout 본인브랜치명
```

작업 후:

```bash
git status
git add .
git commit -m "feat: 커밋내용"
git push origin 본인브랜치명
```

**작업 끝나시면 PR 날리고 톡주세욤! 어떤거 구현한거 pr날렷다고**

GitHub 사용이 어려운 경우:
zip파일로 압축해서 팀장에게 전달해주세요.

압축파일 예시:

```text
명령어_이름.zip
├── command_ls.c
├── command_cd.c
└── README.txt
```

개인 README에는 아래 내용을 적어주세요.(귀찮으면 안해도됨 걍 대충 톡으로 말로 알려주세욤)

```text
담당자: 000
담당 명령어: ls

구현한 옵션:
- 기본 ls
- ls -a
- ls -l
- ls -al

사용한 공통 함수:
- find_child()
- format_permission()
- is_directory()

테스트한 입력:
ls
ls -a
ls -l
ls -al

아직 안 되는 부분:
- 현재 없음
- 또는 통합 후 확인 필요
```

## 커밋 메시지 규칙

커밋 메시지는 아래 형식을 사용합니다.

```text
타입: 변경 내용
```

자주 쓰는 타입:

| 타입 | 의미 |
|---|---|
| `feat` | 기능 추가 |
| `fix` | 버그 수정 |
| `chore` | 폴더 구조, 설정 등 |
| `docs` | 문서 수정 |
| `build` | Makefile 등 빌드 관련 |
| `refactor` | 기능 변화 없는 코드 개선 |

## 실행 방법

최종 실행은 Ubuntu VM에서 아래 방식으로 진행할 예정입니다.

```bash
make clean
make
./mini_os
```

현재 초기 skeleton 단계에서는 일부 파일이 비어 있을 수 있으므로, 전체 실행은 통합 후 진행합니다.

## 주의사항

- 최종 제출본과 시연에 사용하는 파일은 동일해야 합니다.
- 제출 이후 메인 코드 수정은 하지 않습니다.
- 팀장 VM에서 최종적으로 `make clean`, `make`, `./mini_os` 실행을 확인합니다.
- Windows VS Code에서 `pthread.h` 등의 리눅스 전용 라이브러리 관련 빨간 줄이 보일 수 있지만, 최종 Ubuntu 환경에서 확인합니다. 걍 상관없이 개발하시면 됩니다.