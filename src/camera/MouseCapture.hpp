#ifndef CAELUM_MOUSE_CAPTURE_HPP
#define CAELUM_MOUSE_CAPTURE_HPP
struct GLFWwindow;
// Native confinement supplements GLFW relative input; never changes camera math.
void ConfineMouseToWindow(GLFWwindow* window,bool confined);
#endif
