//
// Created by 송서혁 on 2025. 4. 2..
//

#include <platform.h>
#include <stdlib.h>

#if _Cocoa
#include "MacOSPlatform/CocoaPlatform.h"
#endif

PTR64 internalPlatformToolPtr;





WindowInfo defaultWindowInfo() {
    const WindowInfo windowInfo = {
        .flags = 0b0,
        .dimension ={ .width = 1, .height = 1 },
        .title = "Default window title",
    };

    return windowInfo;
}

void freePlatformTool(void) {
    free(internalPlatformToolPtr.asVoidPtr);
}

PlatformTool * allocPlatformTool(WindowInfo windowInfo, FLAGS flags) {
    PlatformTool * platformTool = (PlatformTool *) malloc(sizeof(PlatformTool));

    platformTool->ptrWindowInfo = windowInfo;
    platformTool->windowHandle.asVoidPtr = NULL;
    if ((flags & SHOULD_AUTO_FREE) != 0) {
        platformTool->freeIfCan.asFunction = freePlatformTool;
    }
    else {
        platformTool->freeIfCan.asFunction = skipFree;
    }

    return platformTool;
}


