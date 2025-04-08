#pragma once

typedef union {
    uint32_t asInteger;
    void * asPointer;
    
    
} PTR64 uint64_t;

typedef opequeObject void *;

typedef surface

typedef struct ExtraThinWindow_ {
    const char title;
    struct dim {
        uint32_t width, height, x, y;
    }
    bool isResizable;
    bool isBorderless;
    
    
} ExtraThinWindow;

inline ExtraThineWindow *
