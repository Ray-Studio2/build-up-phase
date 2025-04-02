//
// Created by nefty on 2025-03-22.
//

/**
 *
 * Platform tool for C.
 *
 * only standard keyboard and mouse, win32, Cocoa, X11 is tested.
 *
 * strongly using 64bit platforms. only supports vulkan API.
 *
 * the pure C structs are useful to debug with gdb or lldb.
 * you can see through all data in raw just by watching
 * the address.
 *
 */

#pragma once

#include <inttypes.h>

#define PLEASE 1
#define NOTHENKS 0

#ifdef _WIN32
#endif
#ifdef _Cocoa
    #include "MacOSPlatform/CocoaPlatform.h"
#endif

/**
 * Every pointer hase same bits, all raw pointer values are
 * exactly same a same size of machine default bits.
 *
 * this tool only provides 64bit and all pointers will fit in
 * PTR64.
 *
 * The PTR64 can handle every object in the <simple_platform>.
 * this mean you should understand the type of PTR64 returns
 * by the name. for example allocatePlatformTool will return
 * PTR64. and it's going to point the PlatformTool in the heap.
 *
 * don't try to access with a wrong type. it will cause segmentation fault.
 */

typedef union PTR64_ {
    void * asVoidPtr;
    uint64_t asInteger64;
} PTR64;


/**
 * By declaring function pointer like
 *
 * typedef <type>(*<name>)(<args>)
 *
 * this allows programmer to use function
 * pointer as a type of ANY kind of function
 * unless return type is matching.
 *
 * after this using <name> in struct, union,
 * class(for c++) as available.
 *
 * create variable with {} and use it like a struct.
 */

typedef PTR64 (*FN_PTR64__)(void);
typedef union FN_PTR64_ {
    FN_PTR64__ asFunction;
    uint64_t asInteger64;
} FN_PTR64;

typedef void (*FN_FREE_PTR64__)(void);
typedef union FN_FREE_PTR64_ {
    FN_FREE_PTR64__ asFunction;
    PTR64 asInteger64;
} FN_FREE_PTR64;

/**
 * if you don't have to free, you can use this as default.
 */
void skipFree(void);

#define IS_RESIZABLE 0b1
#define IS_FULLSCREEN 0b10
#define SHOULD_AUTO_FREE 0b100

typedef uint8_t FLAGS;

/**
 * The window Information Object
 *
 * you can fill in some values if you want.
 * otherwise you can use default value from
 * DefaultWindowInfo.
 *
 * after it's done, use the createWindow(WindowInfo) function.
 *
 * YOU CAN USE Smart Pointer!!! it's tested.
 */
typedef struct WindowInfo_ {
    FLAGS flags;
    struct dimension {
        uint32_t width;
        uint32_t height;
    } dimension;
    const char * title;
} WindowInfo;

WindowInfo defaultWindowInfo();

/*
typedef union PTR_WindowInfo_ {
    PTR64 asPointer64;
    WindowInfo asWindowInfo;
} PTR64_WindowInfo;

PTR64_WindowInfo * allocWindowInfo (
    char flags,
    uint32_t width, uint32_t height,
    const char * title,
    FN_FREE_PTR64 freeFunction
);
*/

typedef struct PlatformTool_ {
    WindowInfo ptrWindowInfo;

    PTR64 windowHandle;

    FN_FREE_PTR64 freeIfCan;
} PlatformTool;

typedef union PTR64_PlatformTool_ {
    PTR64 asPointer64;
    PlatformTool * ptr;
} PTR64_PlatformTool;

PlatformTool * allocPlatformTool(WindowInfo windowInfo, FLAGS flags);

#ifdef _WIN32
#endif

#ifdef _Cocoa
#endif

