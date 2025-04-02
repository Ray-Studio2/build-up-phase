//
// Created by nefty on 2025-03-22.
//

#pragma once



namespace platform {

    typedef union PTR64_ {
        uint32_t asValue;
        void * asVoidPtr;
    } PTR64;

    typedef struct window_ {
        
    } window;

    typedef struct PlatformTool_ {
        PTR64 windowHandle;
        
        PlatformTool();
        ~PlatformTool();

        PlatformTool_ createWindow();
    } PlatformTool;

#ifdef Win32
#endif

#ifdef linux
#endif

#ifdef Darwin
    PlatformTool() {
        windowHandle = nullptr;
    };
#endif

}
