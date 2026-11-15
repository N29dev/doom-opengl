#pragma once
struct GLFWwindow;
namespace native_window {
#ifdef __APPLE__
// Cocoa fullscreen is independent of GLFW monitor/exclusive fullscreen.
void observe(GLFWwindow* window);
void unobserve();
bool fullscreen(GLFWwindow* window);
bool transitioning();
bool requestFullscreen(GLFWwindow* window,bool enabled);
#endif
}
