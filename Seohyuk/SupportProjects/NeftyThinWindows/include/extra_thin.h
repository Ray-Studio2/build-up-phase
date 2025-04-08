#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DEFINE_OPAQUE_POINTER(name_) typedef union { uint64_t asIntValue64; void * asPointer; name_ * as##name; } PTR64##name_;

#ifdef _WIN32

#include <windows.h>
DEFINE_OPAQUE_POINTER(HWND)

#endif


//PTR64HWND

typedef struct ExtraThinWindow_ {
    const char * title;
    struct dim_ {
        uint32_t width, height, x, y;
    } dim;
    bool isResizable;
    bool isBorderless;
    
    union windowHandle_ {
        void * asPointer;
        PTR64HWND asWin32Window;
    } windowHandle;
} ExtraThinWindow;

ExtraThinWindow openWindow(const char * title, uint32_t width, uint32_t height, bool isResizable, bool isBorderless);
