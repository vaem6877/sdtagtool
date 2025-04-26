#include <string.h>     // 문자열 처리 (strlen, strcpy 등)
#include <windows.h>    // 윈도우 API 사용 (클립보드 등)
#include <wchar.h>
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {0};

    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TagSelector";

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"창 클래스 등록실패!", L"에러", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowW(
        L"TagSelector",
        L"태그 셀렉터",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    HWND hwndList = CreateWindowW(
        L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        10, 10, 760, 540,
        hwnd, NULL, hInstance, NULL
    );

    const char* prevCategory = NULL;
    wchar_t lineBuffer[1024];
    lineBuffer[0] = L'\0';
    int currentLength = 0;

    for (int i = 0; i < tagCount; i++) {

        if (!prevCategory || strcmp(prevCategory, tags[i].category) != 0) {
            if (wcslen(lineBuffer) > 0) {
                SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
                lineBuffer[0] = L'\0';
                currentLength = 0;
            }

            SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)L"");

            wchar_t categoryLine[256];
            wchar_t formattedCategory[260]; // 대괄호 2글자 추가 공간 고려
            
            MultiByteToWideChar(CP_UTF8, 0, tags[i].category, -1, categoryLine, 256);
            
            // 대괄호로 감싸기
            swprintf(formattedCategory, 260, L"[%s]", categoryLine);
            
            // 리스트박스에 추가
            SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)formattedCategory);

            prevCategory = tags[i].category;

        }

        // (2) 태그 이어붙이기
        wchar_t tagBuffer[256];
        MultiByteToWideChar(CP_UTF8, 0, tags[i].tag, -1, tagBuffer, 256);
        int tagLen = wcslen(tagBuffer);

        int nextLength = currentLength;
        if (currentLength > 0) nextLength += 3;
        nextLength += tagLen;

        if (nextLength >= 80) {
            if (wcslen(lineBuffer) > 0) {
                SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
                lineBuffer[0] = L'\0';
                currentLength = 0;
            }
        }

        if (currentLength > 0) {
            wcscat(lineBuffer, L",  ");
            currentLength += 3;
        }

        wcscat(lineBuffer, tagBuffer);
        currentLength += tagLen;
    }

    if (wcslen(lineBuffer) > 0) {
        SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
    }
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}