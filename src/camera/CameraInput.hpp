#ifndef CAELUM_CAMERA_INPUT_HPP
#define CAELUM_CAMERA_INPUT_HPP
#include "camera/CameraController.hpp"
// Keep GLFW out of the camera math module.
struct GLFWwindow;

// AI-assisted GLFW adapter: translates events/held keys into camera commands.
class CameraInput {
public:
    CameraInput(GLFWwindow* window, CameraController& camera, const CameraFrame& frame);
    ~CameraInput();
    bool Key(int key, int action);
    void MouseButton(int button, int action);
    void Cursor(double x, double y);
    void Scroll(double amount);
    void Focus(bool focused);
    void Update(float dt);
private:
    GLFWwindow* window;
    CameraController& camera;
    const CameraFrame& frame;
    bool looking=false, capturePending=false, rawMotion=false;
    double lastX=0,lastY=0;
    double captureReadyAt=0;
    unsigned captureBatches=0;
    void ReleaseMouse();
};
#endif
