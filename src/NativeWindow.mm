#include "NativeWindow.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#import <AppKit/AppKit.h>

namespace native_window {
namespace {
    NSMutableArray* observers = nil;
    bool inTransition = false;
    double started = 0.0;

    void beginTransition() {
        inTransition = true;
        started = glfwGetTime();
    }
    void endTransition() { inTransition = false; }

    NSWindow* cocoa(GLFWwindow* window) {
        return (NSWindow*)glfwGetCocoaWindow(window);
    }
}
void unobserve() {
    @autoreleasepool {
        for (id token in observers)
            [[NSNotificationCenter defaultCenter] removeObserver:token];
        observers = nil;
        inTransition = false;
    }
}
void observe(GLFWwindow* window) {
    @autoreleasepool {
        unobserve();
        NSWindow* object = cocoa(window);
        if (!object) return;
        observers = [[NSMutableArray alloc] init];
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        for (NSString* name in @[NSWindowWillEnterFullScreenNotification,NSWindowWillExitFullScreenNotification]) {
            id token = [center addObserverForName:name object:object queue:nil usingBlock:^(NSNotification* note) {
                (void)note;
                beginTransition();
            }];
            [observers addObject:token];
        }
        for (NSString* name in @[NSWindowDidEnterFullScreenNotification,NSWindowDidExitFullScreenNotification]) {
            id token = [center addObserverForName:name object:object queue:nil usingBlock:^(NSNotification* note) {
                (void)note;
                endTransition();
            }];
            [observers addObject:token];
        }
    }
}
bool fullscreen(GLFWwindow* window) {
    @autoreleasepool {
        NSWindow* object = cocoa(window);
        return object && (([object styleMask] & NSWindowStyleMaskFullScreen) != 0);
    }
}
bool transitioning() {
    // A rejected OS request must not permanently disable the menu.
    if (inTransition && glfwGetTime()-started>8.0) endTransition();
    return inTransition;
}
bool requestFullscreen(GLFWwindow* window,bool enabled) {
    @autoreleasepool {
        NSWindow* object = cocoa(window);
        if (!object || transitioning() || fullscreen(window)==enabled) return false;
        beginTransition();
        [object toggleFullScreen:nil];
        return true;
    }
}
}
