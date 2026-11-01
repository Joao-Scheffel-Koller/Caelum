#include "camera/CameraInput.hpp"
#include "camera/MouseCapture.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
// FONTE: https://github.com/glfw/glfw/issues/2523
// Moving during capture can queue wrong offsets across multiple event batches.
const double CaptureSettleSeconds=.100;
}

CameraInput::CameraInput(GLFWwindow* window,CameraController& camera,const CameraFrame& frame)
    :window(window),camera(camera),frame(frame) {}
CameraInput::~CameraInput() { ReleaseMouse(); }

void CameraInput::ReleaseMouse() {
    const bool wasCaptured=looking||capturePending;
    looking=false;capturePending=false;
    captureReadyAt=0;captureBatches=0;
    if(wasCaptured)ConfineMouseToWindow(window,false);
    if(rawMotion)glfwSetInputMode(window,GLFW_RAW_MOUSE_MOTION,GLFW_FALSE);
    rawMotion=false;
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
}

bool CameraInput::Key(int key,int action) {
    if(action==GLFW_PRESS) {
        if(key==GLFW_KEY_C || key==GLFW_KEY_L || key==GLFW_KEY_F) {
            ReleaseMouse();
            camera.SetMode(key==GLFW_KEY_C?CameraType::COCKPIT:
                           key==GLFW_KEY_F?CameraType::FREE:CameraType::LOOK_AT,frame);
            return true;
        }
        if(key==GLFW_KEY_HOME) {camera.Reset(frame);return true;}
    }
    // Consuming W/S here prevents throttle changes while exploring the map.
    return camera.Mode()==CameraType::FREE &&
        (key==GLFW_KEY_W || key==GLFW_KEY_S || key==GLFW_KEY_A ||
         key==GLFW_KEY_D || key==GLFW_KEY_Q || key==GLFW_KEY_E);
}

void CameraInput::MouseButton(int button,int action) {
    if(button!=GLFW_MOUSE_BUTTON_RIGHT)return;
    if(action==GLFW_RELEASE) {ReleaseMouse();return;}
    if(action==GLFW_PRESS && glfwGetWindowAttrib(window,GLFW_FOCUSED)) {
        if(looking||capturePending)return;
        // Recenter/confinement callbacks may arrive after this callback returns.
        // FONTE: https://github.com/glfw/glfw/blob/3.3.8/src/win32_window.c
        // Let delayed recenter events settle before arming normal camera input.
        capturePending=true;
        captureReadyAt=glfwGetTime()+CaptureSettleSeconds;
        captureBatches=0;
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        rawMotion=glfwRawMouseMotionSupported()==GLFW_TRUE;
        if(rawMotion)glfwSetInputMode(window,GLFW_RAW_MOUSE_MOTION,GLFW_TRUE);
        ConfineMouseToWindow(window,true);
    }
}

void CameraInput::Cursor(double x,double y) {
    if(!looking)return;
    camera.Look(float(x-lastX),float(y-lastY));
    lastX=x;lastY=y;
}
void CameraInput::Scroll(double amount) {camera.Scroll(float(amount));}
void CameraInput::Focus(bool focused) {if(!focused)ReleaseMouse();}

void CameraInput::Update(float dt) {
    if(!glfwGetWindowAttrib(window,GLFW_FOCUSED)) {
        if(looking||capturePending)ReleaseMouse();
        return;
    }
    // Refresh the client rectangle if the window is moved/resized during a drag.
    if(looking||capturePending)ConfineMouseToWindow(window,true);
    if(capturePending) {
        // Refresh the anchor throughout the short transition, including offsets
        // delivered after the first poll. Never limit deltas once look is armed.
        glfwGetCursorPos(window,&lastX,&lastY);
        ++captureBatches;
        if(captureBatches>=2 && glfwGetTime()>=captureReadyAt) {
            capturePending=false;
            looking=true;
        }
    }
    const auto down=[this](int key){return glfwGetKey(window,key)==GLFW_PRESS?1.0f:0.0f;};
    CameraMovement movement;
    if(camera.Mode()==CameraType::FREE) {
        movement.forward=down(GLFW_KEY_W)-down(GLFW_KEY_S);
        movement.right=down(GLFW_KEY_D)-down(GLFW_KEY_A);
        movement.vertical=down(GLFW_KEY_E)-down(GLFW_KEY_Q);
        movement.fast=down(GLFW_KEY_LEFT_SHIFT)||down(GLFW_KEY_RIGHT_SHIFT);
    } else if(camera.Mode()==CameraType::COCKPIT) {
        movement.forward=down(GLFW_KEY_UP)-down(GLFW_KEY_DOWN);
        movement.right=down(GLFW_KEY_RIGHT)-down(GLFW_KEY_LEFT);
        movement.vertical=down(GLFW_KEY_PAGE_UP)-down(GLFW_KEY_PAGE_DOWN);
    }
    camera.Update(movement,dt,frame);
}
