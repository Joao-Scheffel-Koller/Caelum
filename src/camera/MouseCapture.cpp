#include "camera/MouseCapture.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

void ConfineMouseToWindow(GLFWwindow* window,bool confined) {
#ifdef _WIN32
    // FONTE: https://www.glfw.org/docs/3.3/group__native.html
    // FONTE: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-clipcursor
    if(!confined) {ClipCursor(nullptr);return;}
    if(!glfwGetWindowAttrib(window,GLFW_FOCUSED))return;
    RECT rectangle;
    HWND native=glfwGetWin32Window(window);
    if(!GetClientRect(native,&rectangle))return;
    POINT topLeft={rectangle.left,rectangle.top},bottomRight={rectangle.right,rectangle.bottom};
    if(!ClientToScreen(native,&topLeft)||!ClientToScreen(native,&bottomRight))return;
    rectangle={topLeft.x,topLeft.y,bottomRight.x,bottomRight.y};
    ClipCursor(&rectangle);
#else
    (void)window;(void)confined; // GLFW disabled cursor supplies confinement.
#endif
}
