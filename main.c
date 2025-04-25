#include <stdio.h>
#include <string.h>     // 문자열 처리 (strlen, strcpy 등)
#include <windows.h>    // 윈도우 API 사용 (클립보드 등)
#include "tag.h"

// 클립보드에 문자열 복사하는 함수
typedef const char* cstr;  // 가독성을 위해 const char*에 별칭 지정
void copy_to_clipboard(cstr text) {
    size_t len = strlen(text) + 1;  // 복사할 문자열 길이 계산 (NULL 문자 포함)
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);  // 전역 메모리 블록 할당
    memcpy(GlobalLock(hMem), text, len);  // 문자열 복사 후 메모리 잠금 해제
    GlobalUnlock(hMem);
    OpenClipboard(0);       // 클립보드 열기
    EmptyClipboard();       // 클립보드 초기화
    SetClipboardData(CF_TEXT, hMem);  // 문자열을 클립보드에 설정
    CloseClipboard();       // 클립보드 닫기
}

void print_tag_list(void) {
    printf("\n===== 태그 목록 ====\n\n");
    for (int i = 0; i < tagCount; i++) {
        printf("[%d] %s (%s) - %s\n", i, tags[i].category, tags[i].tag, tags[i].description);
    }
}

int get_user_selections(int selected[], int max_count) {
    char input[1024];

    printf("태그 번호 선택 쉼표 또는 공백으로 구분 : \n");
    fgets(input, sizeof(input), stdin);

    int count = 0;
    char *token = strtok(input, ", \n");

    while (token != NULL && count < max_count) {
        int index = atoi(token);

        if (index >= 0 && index < tagCount) {
            int is_duplicate = 0;
            for (int i = 0; i < count; i++) {
                if (selected[i] == index) {
                    is_duplicate = 1;
                    break;
                }
            }
            if (!is_duplicate) {
                selected[count++] = index;
            }
        }
        token = strtok(NULL, ", \n");
    }
    return count;
}

char* build_tag_string(int selected[], int count) {
    char *result = (char *)malloc(2048);
    if (!result) return NULL;

    result[0] = '\0';

    for (int i = 0; i < count; i++) {
        strcat(result, tags[selected[i]].tag);
        strcat(result, ",");
    }

    return result;
}




int main() {
    SetConsoleOutputCP(CP_UTF8);
    while (1) {
        print_tag_list();

        int selected[64];
        int count = get_user_selections(selected, 64);

        if (count == 0) {
            printf("유효한 태그가 없습니다.\n");
            continue;
        }

        char *result = build_tag_string(selected, count);
        if (result) {
            copy_to_clipboard(result);
            printf("\n[클립보드 복사완료] -> %s\n", result);
            free(result);
        }
        
    }
    return 0;    
}