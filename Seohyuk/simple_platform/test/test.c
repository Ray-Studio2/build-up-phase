//
//  test.c
//  test
//
//  Created by 송서혁 on 4/2/25.
//

#include "platform.h"

//typedef PTR64 (*FN_PTR64__)(void);


int main(int argc, const char * argv[]) {
    WindowInfo windowInfo = defaultWindowInfo();
    PlatformTool * platformTool = allocPlatformTool(windowInfo, PLEASE);

    return 0;
}
