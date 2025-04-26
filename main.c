#include <windows.h>
#include <wchar.h>
#include "tag.h"
#include <stdbool.h>

#define ID_COPY_BUTTON 5000
#define ID_CLEAR_BUTTON 5001

HWND hwndScrollContainer;
HWND hwndScrollArea;
HWND hwndEdit;
int totalHeight = 0;

LRESULT CALLBACK TagScrollAreaProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND) {
        HWND hwndParent = GetParent(hwnd);
        if (hwndParent) {
            SendMessageW(hwndParent, WM_COMMAND, wParam, lParam);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK TagScrollProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_VSCROLL:
        case WM_MOUSEWHEEL: {
            SCROLLINFO si = { sizeof(SCROLLINFO), .fMask = SIF_ALL };
            GetScrollInfo(hwnd, SB_VERT, &si);

            int newPos = si.nPos;
            if (msg == WM_VSCROLL) {
                switch (LOWORD(wParam)) {
                    case SB_LINEUP: newPos -= 20; break;
                    case SB_LINEDOWN: newPos += 20; break;
                    case SB_PAGEUP: newPos -= si.nPage; break;
                    case SB_PAGEDOWN: newPos += si.nPage; break;
                    case SB_THUMBTRACK: newPos = HIWORD(wParam); break;
                }
            } else {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                newPos -= (zDelta / WHEEL_DELTA) * 20;
            }

            newPos = max(si.nMin, min(newPos, si.nMax - (int)si.nPage + 1));
            si.nPos = newPos;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            MoveWindow(hwndScrollArea, 0, -newPos, 500, totalHeight, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            HWND hwndParent = GetParent(hwnd);
            if (hwndParent) {
                SendMessageW(hwndParent, WM_COMMAND, wParam, lParam);
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int id = LOWORD(wParam);

            if (id >= 1000 && id < 5000) { // 태그 버튼 클릭
                int tagIndex = id - 1000;

                wchar_t tagText[256];
                MultiByteToWideChar(CP_UTF8, 0, tags[tagIndex].tag, -1, tagText, 256);

                int length = GetWindowTextLengthW(hwndEdit);
                wchar_t* currentText = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
                if (!currentText) break;
                GetWindowTextW(hwndEdit, currentText, length + 1);

                // 현재 태그 개수 검사
                wchar_t* countText = _wcsdup(currentText);
                int tagCountNow = 0;
                wchar_t* context = NULL;
                wchar_t* token = wcstok(countText, L",", &context);
                while (token) {
                    tagCountNow++;
                    token = wcstok(NULL, L",", &context);
                }
                free(countText);

                if (tagCountNow >= 5) {
                    free(currentText);
                    MessageBoxW(hwnd, L"최대 5개까지만 추가할 수 있습니다.", L"경고", MB_OK | MB_ICONWARNING);
                    break;
                }

                // 중복 검사
                wchar_t* checkText = _wcsdup(currentText);
                bool isDuplicate = false;
                context = NULL;
                token = wcstok(checkText, L",", &context);
                while (token) {
                    while (*token == L' ') token++;
                    if (wcscmp(token, tagText) == 0) {
                        isDuplicate = true;
                        break;
                    }
                    token = wcstok(NULL, L",", &context);
                }
                free(checkText);

                if (isDuplicate) {
                    free(currentText);
                    MessageBoxW(hwnd, L"이미 추가된 태그입니다.", L"알림", MB_OK | MB_ICONINFORMATION);
                    break;
                }

                // 정상 추가
                wchar_t* newText = (wchar_t*)malloc((length + wcslen(tagText) + 4) * sizeof(wchar_t));
                if (!newText) {
                    free(currentText);
                    break;
                }
                wcscpy(newText, currentText);
                wcscat(newText, tagText);
                wcscat(newText, L", ");
                SetWindowTextW(hwndEdit, newText);

                free(currentText);
                free(newText);

                // 복사
                if (OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    size_t sizeInBytes = (wcslen(tagText) + 3) * sizeof(wchar_t);
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
                    if (hMem) {
                        wchar_t* memPtr = (wchar_t*)GlobalLock(hMem);
                        if (memPtr) {
                            swprintf(memPtr, (wcslen(tagText) + 3), L"%s, ", tagText);
                            GlobalUnlock(hMem);
                            SetClipboardData(CF_UNICODETEXT, hMem);
                        }
                    }
                    CloseClipboard();
                }
            } else if (id == ID_COPY_BUTTON) { // 복사 버튼
                int length = GetWindowTextLengthW(hwndEdit);
                if (length > 0) {
                    wchar_t* text = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
                    if (!text) break;
                    GetWindowTextW(hwndEdit, text, length + 1);

                    if (OpenClipboard(hwnd)) {
                        EmptyClipboard();
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (length + 1) * sizeof(wchar_t));
                        if (hMem) {
                            wchar_t* memPtr = (wchar_t*)GlobalLock(hMem);
                            if (memPtr) {
                                memcpy(memPtr, text, (length + 1) * sizeof(wchar_t));
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            }
                        }
                        CloseClipboard();
                    }
                    free(text);
                    SetWindowTextW(hwndEdit, L"");
                }
            } else if (id == ID_CLEAR_BUTTON) { // 초기화 버튼
                SetWindowTextW(hwndEdit, L"");
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    const int windowWidth = 500;
    const int windowHeight = 700;
    const int fixedAreaHeight = 100;
    const int buttonWidth = 100;
    const int buttonHeight = 30;
    const int gapX = 10;
    const int gapY = 10;
    const int buttonsPerRow = 4;

    // 메인 윈도우 클래스 등록
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TagSelector";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"TagSelector", L"태그 셀렉터", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 텍스트박스 및 버튼 추가
    hwndEdit = CreateWindowW(L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        10, 10, 480, 50, hwnd, NULL, hInstance, NULL);

    CreateWindowW(L"BUTTON", L"복사", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        10, 65, 100, 30, hwnd, (HMENU)ID_COPY_BUTTON, hInstance, NULL);

    CreateWindowW(L"BUTTON", L"초기화", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        120, 65, 100, 30, hwnd, (HMENU)ID_CLEAR_BUTTON, hInstance, NULL);

    // 스크롤 컨테이너 클래스 등록
    WNDCLASSW wcScroll = {0};
    wcScroll.lpfnWndProc = TagScrollProc;
    wcScroll.hInstance = hInstance;
    wcScroll.lpszClassName = L"TagScrollContainer";
    wcScroll.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcScroll.style = CS_VREDRAW | CS_HREDRAW;
    RegisterClassW(&wcScroll);

    // 스크롤 영역 클래스 등록
    WNDCLASSW wcArea = {0};
    wcArea.lpfnWndProc = TagScrollAreaProc;
    wcArea.hInstance = hInstance;
    wcArea.lpszClassName = L"TagScrollArea";
    wcArea.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcArea.style = CS_VREDRAW | CS_HREDRAW;
    RegisterClassW(&wcArea);

    // 스크롤 컨테이너 및 영역 생성
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int containerHeight = rcClient.bottom - fixedAreaHeight;

    hwndScrollContainer = CreateWindowW(L"TagScrollContainer", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
        0, fixedAreaHeight, windowWidth, containerHeight, hwnd, NULL, hInstance, NULL);

    hwndScrollArea = CreateWindowW(L"TagScrollArea", NULL, WS_CHILD | WS_VISIBLE,
        0, 0, windowWidth, 0, hwndScrollContainer, NULL, hInstance, NULL);

    int x = 10, y = 10;
    int buttonsInRow = 0;
    const char* prevCategory = NULL;

    for (int i = 0; i < tagCount; i++) {
        if (!prevCategory || strcmp(prevCategory, tags[i].category) != 0) {
            if (buttonsInRow > 0) {
                x = 10;
                y += buttonHeight + gapY;
                buttonsInRow = 0;
            }

            wchar_t categoryText[256];
            MultiByteToWideChar(CP_UTF8, 0, tags[i].category, -1, categoryText, 256);

            CreateWindowW(L"STATIC", categoryText, WS_CHILD | WS_VISIBLE,
                x, y, 300, 20, hwndScrollArea, NULL, hInstance, NULL);

            y += 30;
            x = 10;
            prevCategory = tags[i].category;
        }

        wchar_t tagText[256];
        MultiByteToWideChar(CP_UTF8, 0, tags[i].tag, -1, tagText, 256);

        CreateWindowW(L"BUTTON", tagText, WS_CHILD | WS_VISIBLE | BS_LEFT,
            x, y, buttonWidth, buttonHeight, hwndScrollArea, (HMENU)(uintptr_t)(1000 + i), hInstance, NULL);

        x += buttonWidth + gapX;
        buttonsInRow++;

        if (buttonsInRow >= buttonsPerRow) {
            x = 10;
            y += buttonHeight + gapY;
            buttonsInRow = 0;
        }
    }

    if (buttonsInRow > 0) y += buttonHeight + gapY;

    totalHeight = y;
    MoveWindow(hwndScrollArea, 0, 0, windowWidth, totalHeight, TRUE);

    RECT rcContainer;
    GetClientRect(hwndScrollContainer, &rcContainer);
    containerHeight = rcContainer.bottom;

    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (totalHeight > containerHeight) ? totalHeight - 1 : 0;
    si.nPage = containerHeight;
    si.nPos = 0;
    SetScrollInfo(hwndScrollContainer, SB_VERT, &si, TRUE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}