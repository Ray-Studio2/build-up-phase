//
//  CocoaPlatform.m
//  CocoaPlatform
//
//  Created by 송서혁 on 4/2/25.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import "CocoaPlatform.h"
#include "platform.h"

@interface WindowView: NSView
@end

void skipFree(void) {
    asm volatile("nop" ::: "memory");
}
