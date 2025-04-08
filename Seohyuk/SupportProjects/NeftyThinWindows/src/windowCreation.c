
#include <stdio.h>

#include "extra_thin.h"

#ifdef _Cocoa

#endif
void sayHello(void) {
    printf("Hello!");
}

PtrTheWindow createWindowDefault() {
}
PtrTheWindow createWindowTitled(const char * title){}
PtrTheWindow createWindowTitleDim(const char* title, uint32_t width, uint32_t height){}
PtrTheWindow createWindowTitlePosDim(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height){}

void destroyWindow(PtrTheWindow window) {

}
