
#include <stdio.h>

#include "extra_thin.h"

#ifdef _WIN32

#include <windows.h>

HWND * openWindowWin32(const char * title, uint32_t width, uint32_t height) {
    WNDCLASSEX wc = {};

}

#endif

ExtraThinWindow openWindow(const char * title, uint32_t width, uint32_t height, bool isResizable, bool isBorderless) {
    ExtraThinWindow window = {
    .title = title,
    .dim = {
        .width = width,
        .height = height,
        .x = 0,
        .y = 0,
    },
    .isResizable = isResizable,
    .isBorderless = isBorderless,
    .windowHandle = NULL
    };

    return window;
}
