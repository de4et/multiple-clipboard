#include <stdio.h>
#include <windows.h>
#include <wchar.h>


#define WCHAR_SIZE 2
#define MAX_CLIPBOARD_SIZE 4 * 1024 * 1024 / WCHAR_SIZE
#define MAX_CLIPBOARDS 9

wchar_t clipboards[MAX_CLIPBOARDS][MAX_CLIPBOARD_SIZE];
int clipboards_amount = 0;

int curr = MAX_CLIPBOARDS - 1;

DWORD last_sn;
BOOL isClipboardChanged() {
    // Get last_sn from GetClipboardSequenceNumber()
    DWORD current_sn = GetClipboardSequenceNumber();

    if (last_sn != current_sn) {
        return 1;
    }
    return 0;
}

void UpdateSN() {
    last_sn = GetClipboardSequenceNumber();
}

wchar_t* getClipboardData() {
    HGLOBAL hglb;
    wchar_t *u_text;

    while (!OpenClipboard(NULL)) {}

    hglb = GetClipboardData(CF_UNICODETEXT);
    if (!hglb) {
        perror("\nCould not GetClipboardData!");
        return NULL;
    }

    u_text = GlobalLock(hglb);

    GlobalUnlock(hglb);
    CloseClipboard();
    return wcsdup(u_text);
}


void addClipboardData(wchar_t *data) {
    for (int i = 0; i < MAX_CLIPBOARDS-1; i++) {
        wcscpy(clipboards[i], clipboards[i+1]);
    }
    wcscpy(clipboards[MAX_CLIPBOARDS-1], data);
}

HGLOBAL setClipboardData(wchar_t *data) {
    int len = wcslen(data);

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE,  (len + 1) * sizeof(WCHAR));

    LPWSTR dst = GlobalLock(mem);
    memcpy(dst, data, len * sizeof(WCHAR));
    dst[len] = (WCHAR) 0;
    GlobalUnlock(mem);

    while (!OpenClipboard(NULL)) {}
    EmptyClipboard();
    HGLOBAL gbl = SetClipboardData(CF_UNICODETEXT, mem);

    if (!gbl) {
        printf("\nCould not SetClipboardData!");
    }

    CloseClipboard();
    return gbl;

}

HHOOK hook;
int ctrl_pressed = 0;
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam){
    KBDLLHOOKSTRUCT *st = lParam;

    switch (wParam) {
        case WM_KEYDOWN:
            switch (st->vkCode) {
                case VK_LCONTROL:
                    ctrl_pressed = 1;
                break;

                case VK_NUMPAD4:
                    if (!ctrl_pressed || curr == 0 || clipboards[curr - 1][0] == '\0') break;

                    curr--;
                    setClipboardData(clipboards[curr]);
                    UpdateSN();
                break;

                case VK_NUMPAD6:
                    if (!ctrl_pressed || curr == MAX_CLIPBOARDS - 1 || clipboards[curr + 1][0] == '\0') break;

                    curr++;
                    setClipboardData(clipboards[curr]);
                    UpdateSN();
                break;
            }
        break;

        case WM_KEYUP:
            switch (st->vkCode) {
                case VK_LCONTROL:
                    ctrl_pressed = 0;
                break;
            }
        break;
    }

    return CallNextHookEx(hook,code,wParam,lParam);
}


int main() {
    // Initing and hiding window
    HWND window;
    MSG msg;
    AllocConsole();
    window = FindWindowA("ConsoleWindowClass",NULL);
    ShowWindow(window,0);

    // Setting hook to track keys pressed
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    while (1) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        else {
            // check if clipboard did not changed
            if (!isClipboardChanged()) {continue;}

            // get clipboard data
            wchar_t *data = getClipboardData();
            if (data == NULL) {
                printf("\nClipboardData is NULL");
                return 1;
            }

            // add clipboard data to an array
            addClipboardData(data);

            // update last_sn
            UpdateSN();
        }
    }
    return 0;
}


