// AI-assisted regression tests for the GLFW adapter, without a window or GPU.
#include "camera/CameraInput.hpp"
#include "camera/MouseCapture.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// FONTE: codigo base do professor, include/matrices.h.
#include "matrices.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
std::array<int,GLFW_KEY_LAST+1> keys{};
bool focused=true;
bool confined=false;
int cursorMode=GLFW_CURSOR_NORMAL;
bool rawSupported=true,rawEnabled=false,synchronousWarp=false;
double cursorX=0,cursorY=0;
double clockSeconds=0;
CameraInput* callbackInput=nullptr;
int failures=0;

void Check(bool condition,const char* message) {
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",message);++failures;}
}
bool Near(const glm::vec3& a,const glm::vec3& b,float tolerance=1e-4f) {
    return glm::length(a-b)<tolerance;
}
bool SamePose(const CameraPose& a,const CameraPose& b) {
    return Near(a.position,b.position)&&Near(a.forward,b.forward)&&Near(a.up,b.up);
}
void Motion(CameraInput& input,double x,double y) {
    // Native GLFW updates the virtual position before issuing its callback.
    cursorX=x;cursorY=y;
    input.Cursor(x,y);
}
struct Fixture {
    CameraFrame frame;
    CameraController camera;
    CameraInput input;
    Fixture():input(reinterpret_cast<GLFWwindow*>(1),camera,frame) {
        keys.fill(GLFW_RELEASE);focused=true;cursorMode=GLFW_CURSOR_NORMAL;
        rawSupported=true;rawEnabled=false;synchronousWarp=false;cursorX=0;cursorY=0;clockSeconds=0;callbackInput=nullptr;
    }
    void BeginLook() {
        input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS);
        input.Update(0); // End the capture event batch, as in the real main loop.
        clockSeconds+=.101;
        input.Update(0); // The short transition has elapsed; normal look can start.
    }
};

void ModeAndThrottleRouting() {
    Fixture f;
    Check(!f.input.Key(GLFW_KEY_W,GLFW_PRESS),"orbit leaves W for aircraft throttle");
    Check(!f.input.Key(GLFW_KEY_S,GLFW_PRESS),"orbit leaves S for aircraft throttle");
    Check(f.input.Key(GLFW_KEY_C,GLFW_PRESS)&&f.camera.Mode()==CameraType::COCKPIT,
          "C selects cockpit");
    Check(!f.input.Key(GLFW_KEY_W,GLFW_PRESS),"cockpit leaves W for aircraft throttle");
    Check(!f.input.Key(GLFW_KEY_S,GLFW_REPEAT),"cockpit leaves S for aircraft throttle");
    const CameraPose cockpit=f.camera.Pose(f.frame);
    Check(f.input.Key(GLFW_KEY_F,GLFW_PRESS)&&f.camera.Mode()==CameraType::FREE,
          "F selects free camera");
    Check(SamePose(cockpit,f.camera.Pose(f.frame)),"entering free camera has no view jump");
    for(int action:{GLFW_PRESS,GLFW_REPEAT,GLFW_RELEASE}) {
        Check(f.input.Key(GLFW_KEY_W,action),"free consumes every W event");
        Check(f.input.Key(GLFW_KEY_S,action),"free consumes every S event");
    }
    Check(!f.input.Key(GLFW_KEY_R,GLFW_PRESS),"unrelated shortcut remains available");
    const CameraPose detached=f.camera.Pose(f.frame);
    f.frame.aircraftModel=Matrix_Translate(300,20,-50);
    f.frame.orbitTarget=glm::vec3(300,24,-50);
    f.input.Key(GLFW_KEY_F,GLFW_PRESS);
    f.input.Key(GLFW_KEY_F,GLFW_REPEAT);
    Check(SamePose(detached,f.camera.Pose(f.frame)),"repeated F cannot recapture a moving aircraft");
    Check(f.input.Key(GLFW_KEY_L,GLFW_PRESS)&&f.camera.Mode()==CameraType::LOOK_AT,
          "L selects orbit");
    Check(!f.input.Key(GLFW_KEY_W,GLFW_PRESS),"leaving free restores throttle routing");
}

void MouseCaptureAndFocus() {
    Fixture f;
    f.input.Key(GLFW_KEY_C,GLFW_PRESS);
    const CameraPose initial=f.camera.Pose(f.frame);
    Motion(f.input,900,500);
    f.input.MouseButton(GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS);
    Motion(f.input,1000,800);
    Check(SamePose(initial,f.camera.Pose(f.frame)),"mouse cannot steer camera without right button");
    Check(cursorMode==GLFW_CURSOR_NORMAL,"left press does not capture cursor");

    f.BeginLook();
    Check(cursorMode==GLFW_CURSOR_DISABLED,"right press captures cursor for unlimited look");
    Check(confined,"right press confines cursor to the game client area");
    Check(rawEnabled,"supported raw mouse motion bypasses OS acceleration while captured");
    Motion(f.input,1000,800);
    Check(SamePose(initial,f.camera.Pose(f.frame)),"capture anchor stays at the current virtual cursor position");
    Motion(f.input,1030,810);
    const CameraPose turned=f.camera.Pose(f.frame);
    Check(!Near(initial.forward,turned.forward),"first real right-held motion turns camera without a discarded event");
    Check(Near(initial.position,turned.position),"looking never moves the eyes");

    f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_RELEASE);
    Check(cursorMode==GLFW_CURSOR_NORMAL,"right release restores cursor");
    Check(!confined,"right release removes native confinement");
    Check(!rawEnabled,"right release disables raw mouse motion");
    Motion(f.input,-8000,3000);
    Check(SamePose(turned,f.camera.Pose(f.frame)),"motion after release is ignored");
    f.BeginLook();
    Motion(f.input,-8000,3000);
    Check(SamePose(turned,f.camera.Pose(f.frame)),"recapture starts with a fresh anchor");
    focused=false;
    f.input.Focus(false);
    Check(cursorMode==GLFW_CURSOR_NORMAL,"focus loss releases captured cursor");
    Check(!confined,"focus loss removes native confinement");
    Motion(f.input,900,700);
    Check(SamePose(turned,f.camera.Pose(f.frame)),"focus loss cancels look drag");
    f.BeginLook();
    Check(cursorMode==GLFW_CURSOR_NORMAL,"unfocused window cannot capture cursor");
    focused=true;
    f.input.Focus(true);
    Motion(f.input,0,0);
    Check(SamePose(turned,f.camera.Pose(f.frame)),"focus recovery does not resume stale drag");

    f.BeginLook();
    f.input.Key(GLFW_KEY_F,GLFW_PRESS);
    Check(cursorMode==GLFW_CURSOR_NORMAL,"mode switch also releases cursor capture");
}

void FreeMovement() {
    Fixture f;
    f.input.Key(GLFW_KEY_F,GLFW_PRESS);
    const CameraPose initial=f.camera.Pose(f.frame);
    keys[GLFW_KEY_W]=GLFW_PRESS;
    f.input.Update(.2f);
    CameraPose after=f.camera.Pose(f.frame);
    Check(Near(after.position,initial.position+initial.forward*10.0f),
          "held W moves along look direction at 50 m/s using dt");
    f.input.Key(GLFW_KEY_HOME,GLFW_PRESS);
    const glm::vec3 diagonalStart=f.camera.Pose(f.frame).position;
    keys[GLFW_KEY_D]=GLFW_PRESS;
    f.input.Update(.2f);
    Check(std::abs(glm::length(f.camera.Pose(f.frame).position-diagonalStart)-10)<1e-3f,
          "W+D is not faster than W alone");

    for(int shift:{GLFW_KEY_LEFT_SHIFT,GLFW_KEY_RIGHT_SHIFT}) {
        f.input.Key(GLFW_KEY_HOME,GLFW_PRESS);
        keys.fill(GLFW_RELEASE);keys[GLFW_KEY_W]=GLFW_PRESS;keys[shift]=GLFW_PRESS;
        const glm::vec3 start=f.camera.Pose(f.frame).position;
        f.input.Update(.2f);
        Check(std::abs(glm::length(f.camera.Pose(f.frame).position-start)-50)<1e-3f,
              "either Shift key enables five-times travel speed");
    }
    keys.fill(GLFW_RELEASE);
    keys[GLFW_KEY_E]=GLFW_PRESS;
    const glm::vec3 rise=f.camera.Pose(f.frame).position;
    f.input.Update(.2f);
    Check(Near(f.camera.Pose(f.frame).position,rise+glm::vec3(0,10,0)),
          "E rises along world vertical");
    keys.fill(GLFW_RELEASE);keys[GLFW_KEY_Q]=GLFW_PRESS;
    f.input.Update(.2f);
    Check(Near(f.camera.Pose(f.frame).position,rise),"Q reverses vertical movement");
    keys.fill(GLFW_RELEASE);keys[GLFW_KEY_W]=GLFW_PRESS;keys[GLFW_KEY_S]=GLFW_PRESS;
    f.input.Update(.2f);
    Check(Near(f.camera.Pose(f.frame).position,rise),"opposite held directions cancel");

    keys.fill(GLFW_RELEASE);keys[GLFW_KEY_W]=GLFW_PRESS;
    focused=false;f.input.Focus(false);f.input.Update(1);
    Check(Near(f.camera.Pose(f.frame).position,rise),"unfocused held keys do not move camera");
    focused=true;
    const float originalSpeed=f.camera.FreeSpeed();
    f.input.Scroll(2);
    Check(f.camera.FreeSpeed()>originalSpeed,"free scroll adjusts travel speed");
    f.input.Key(GLFW_KEY_HOME,GLFW_PRESS);
    Check(std::abs(f.camera.FreeSpeed()-50)<1e-5f,"Home restores free travel speed");
}

void CockpitMovementAndReset() {
    Fixture f;
    f.input.Key(GLFW_KEY_C,GLFW_PRESS);
    const CameraPose initial=f.camera.Pose(f.frame);
    keys[GLFW_KEY_W]=GLFW_PRESS;
    f.input.Update(1);
    Check(SamePose(initial,f.camera.Pose(f.frame)),"cockpit W does not translate head");
    keys.fill(GLFW_RELEASE);
    keys[GLFW_KEY_UP]=GLFW_PRESS;keys[GLFW_KEY_RIGHT]=GLFW_PRESS;keys[GLFW_KEY_PAGE_UP]=GLFW_PRESS;
    f.input.Update(10);
    const glm::vec3 offset=f.camera.Pose(f.frame).position-initial.position;
    Check(offset.x>0&&offset.y>0&&offset.z>0,"arrows and Page Up move head in all local axes");
    Check(offset.x<=.2001f&&offset.y<=.1201f&&offset.z<=.1801f,
          "held cockpit movement cannot escape cabin bounds");
    f.BeginLook();
    Motion(f.input,100,100);Motion(f.input,160,160);
    Check(f.input.Key(GLFW_KEY_HOME,GLFW_PRESS),"Home is consumed by camera input");
    Check(SamePose(initial,f.camera.Pose(f.frame)),"Home centers cockpit head and resets look");
    keys.fill(GLFW_RELEASE);
    keys[GLFW_KEY_DOWN]=GLFW_PRESS;keys[GLFW_KEY_LEFT]=GLFW_PRESS;keys[GLFW_KEY_PAGE_DOWN]=GLFW_PRESS;
    f.input.Update(10);
    const glm::vec3 negative=f.camera.Pose(f.frame).position-initial.position;
    Check(negative.x<0&&negative.y<0&&negative.z<0,
          "opposite arrows and Page Down move head in opposite local axes");
}

glm::vec2 ScreenPoint(const CameraPose& pose,const glm::vec3& world) {
    // Use the exact teacher matrices used in main, not an alternative lookAt.
    const glm::mat4 view=Matrix_Camera_View(glm::vec4(pose.position,1),
        glm::vec4(pose.forward,0),glm::vec4(pose.up,0));
    const glm::mat4 projection=Matrix_Perspective(pose.fieldOfView,16.0f/9.0f,
        -pose.nearPlane,-pose.farPlane);
    const glm::vec4 clip=projection*view*glm::vec4(world,1);
    return glm::vec2(clip)/clip.w;
}

void MouseAxesThroughActualView() {
    for(int key:{GLFW_KEY_L,GLFW_KEY_C,GLFW_KEY_F}) {
        Fixture f;
        // Real runway heading, coordinate magnitudes and orbit target.
        f.frame.aircraftModel[0]=glm::vec4(.05899537f,0,.99825826f,0);
        f.frame.aircraftModel[1]=glm::vec4(0,1,0,0);
        f.frame.aircraftModel[2]=glm::vec4(-.99825826f,0,.05899537f,0);
        f.frame.aircraftModel[3]=glm::vec4(5237.1933f,0,6295.7127f,1);
        f.frame.orbitTarget=glm::vec3(5237.1933f,4,6295.7127f);
        f.input.Key(key,GLFW_PRESS);
        const CameraPose original=f.camera.Pose(f.frame);
        // A point farther than the orbit target detects orbit movement too.
        const glm::vec3 landmark=original.position+original.forward*100.0f;
        const glm::vec2 before=ScreenPoint(original,landmark);
        Motion(f.input,600,400);
        f.BeginLook();
        Motion(f.input,610,400); // Small angle: avoid perspective's second-order coupling.
        const glm::vec2 horizontal=ScreenPoint(f.camera.Pose(f.frame),landmark)-before;
        Check(std::abs(horizontal.x)>.00001f,"pure horizontal mouse changes horizontal screen coordinate");
        Check(std::abs(f.camera.Pose(f.frame).forward.y-original.forward.y)<.0001f,
              "horizontal mouse preserves pitch");
        Check(std::abs(horizontal.y)<.0005f,"pure horizontal mouse does not turn into vertical motion");
        f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_RELEASE);
        f.camera.Reset(f.frame);
        Motion(f.input,600,400);
        f.BeginLook();
        Motion(f.input,600,390); // Only vertical physical motion.
        const glm::vec2 vertical=ScreenPoint(f.camera.Pose(f.frame),landmark)-before;
        Check(std::abs(vertical.y)>.00001f,"pure vertical mouse changes vertical screen coordinate");
        Check(std::abs(vertical.x)<.0005f,"pure vertical mouse does not turn into horizontal motion");
    }
}

void RawMotionFallbackAndCaptureWarp() {
    Fixture f;
    f.input.Key(GLFW_KEY_C,GLFW_PRESS);
    rawSupported=false;
    f.BeginLook();
    Check(!rawEnabled,"unsupported raw motion falls back to GLFW virtual cursor");
    const CameraPose initial=f.camera.Pose(f.frame);
    Motion(f.input,100,0);
    Check(!Near(initial.forward,f.camera.Pose(f.frame).forward),"fallback applies the first real motion");
    f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_RELEASE);
    f.camera.Reset(f.frame);
    rawSupported=true;synchronousWarp=true;callbackInput=&f.input;
    f.BeginLook();
    Check(SamePose(initial,f.camera.Pose(f.frame)),"capture-generated callback does not rotate camera");
    const double anchorX=cursorX,anchorY=cursorY;
    Motion(f.input,anchorX+100,anchorY);
    Check(!Near(initial.forward,f.camera.Pose(f.frame).forward),"post-capture anchor applies first real delta");
    synchronousWarp=false;callbackInput=nullptr;
    f.input.Focus(false);
    Check(!rawEnabled,"focus loss releases raw capture too");
}

void QueuedCaptureMotion() {
    for(int key:{GLFW_KEY_L,GLFW_KEY_C,GLFW_KEY_F})for(bool raw:{false,true}) {
        Fixture f;
        f.input.Key(key,GLFW_PRESS);
        rawSupported=raw;
        Motion(f.input,1150,640);
        const CameraPose initial=f.camera.Pose(f.frame);
        f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS);
        // OS recenter/confinement events can be queued after the button callback.
        Motion(f.input,640,360);
        Motion(f.input,2200,-800);
        Motion(f.input,650,365);
        Check(SamePose(initial,f.camera.Pose(f.frame)),
              "queued capture events must not turn or reposition any camera");
        f.input.Update(0); // main calls Update after draining glfwPollEvents.
        Check(SamePose(initial,f.camera.Pose(f.frame)),"first capture batch preserves the view");
        // Moving-click bug: the offset and its correction can arrive in later
        // event batches, after the first Update has already completed.
        clockSeconds=.010;
        Motion(f.input,2200,-800);
        f.input.Update(0);
        Check(SamePose(initial,f.camera.Pose(f.frame)),"late capture offset must not turn any camera");
        clockSeconds=.099;
        Motion(f.input,650,365);
        f.input.Update(0);
        Check(SamePose(initial,f.camera.Pose(f.frame)),"late counter-offset must not turn any camera");
        clockSeconds=.101;
        f.input.Update(0);
        CameraController expected=f.camera;
        expected.Look(12,-7);
        Motion(f.input,662,358);
        Check(SamePose(expected.Pose(f.frame),f.camera.Pose(f.frame)),
              "first post-transition motion applies only its actual delta");
        expected.Look(1200,800);
        Motion(f.input,1862,1158);
        Check(SamePose(expected.Pose(f.frame),f.camera.Pose(f.frame)),
              "fast movement after settling keeps normal sensitivity without a delta cap");
    }
    for(bool loseFocus:{false,true}) {
        Fixture f;
        const CameraPose initial=f.camera.Pose(f.frame);
        f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS);
        if(loseFocus) {focused=false;f.input.Focus(false);}
        else f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_RELEASE);
        focused=true;
        clockSeconds=1;
        f.input.Update(0);
        f.input.Update(0);
        Motion(f.input,5000,-2000);
        Check(SamePose(initial,f.camera.Pose(f.frame)),
              "release or focus loss cancels pending capture without resuming look");
        Check(!confined&&!rawEnabled&&cursorMode==GLFW_CURSOR_NORMAL,
              "pending capture releases native and GLFW restrictions");
    }
    {
        Fixture f;
        clockSeconds=100;
        const CameraPose initial=f.camera.Pose(f.frame);
        f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS);
        clockSeconds=101;
        f.input.Update(1000); // A slow frame must still drain a second event batch.
        Motion(f.input,2000,-1000);
        Check(SamePose(initial,f.camera.Pose(f.frame)),
              "long elapsed time cannot bypass the second capture event batch");
        f.input.Update(0);
        CameraController expected=f.camera;
        expected.Look(12,-7);
        Motion(f.input,2012,-1007);
        Check(SamePose(expected.Pose(f.frame),f.camera.Pose(f.frame)),
              "capture resumes normally after settling even after a slow frame");
    }
}

void DestructorReleasesCapture() {
    {
        Fixture f;
        f.input.MouseButton(GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS);
        Check(cursorMode==GLFW_CURSOR_DISABLED,"capture active before adapter destruction");
    }
    Check(cursorMode==GLFW_CURSOR_NORMAL,"adapter destruction releases cursor");
}
}

// Fake implementations match GLFW's C linkage; no GLFW library is linked.
extern "C" void glfwSetInputMode(GLFWwindow*,int mode,int value) {
    if(mode==GLFW_CURSOR) {
        cursorMode=value;
        if(value==GLFW_CURSOR_DISABLED && synchronousWarp) {
            cursorX+=300;cursorY+=200;
            if(callbackInput)callbackInput->Cursor(cursorX,cursorY);
        }
    }
    if(mode==GLFW_RAW_MOUSE_MOTION)rawEnabled=value==GLFW_TRUE;
}
extern "C" int glfwRawMouseMotionSupported() {return rawSupported?GLFW_TRUE:GLFW_FALSE;}
extern "C" double glfwGetTime() {return clockSeconds;}
extern "C" void glfwGetCursorPos(GLFWwindow*,double* x,double* y) {*x=cursorX;*y=cursorY;}
extern "C" int glfwGetWindowAttrib(GLFWwindow*,int attribute) {
    return attribute==GLFW_FOCUSED&&focused?GLFW_TRUE:GLFW_FALSE;
}
extern "C" int glfwGetKey(GLFWwindow*,int key) {
    return key>=0&&key<=GLFW_KEY_LAST?keys[size_t(key)]:GLFW_RELEASE;
}

void ConfineMouseToWindow(GLFWwindow*,bool value) {confined=value;}

int main() {
    ModeAndThrottleRouting();
    MouseCaptureAndFocus();
    FreeMovement();
    CockpitMovementAndReset();
    DestructorReleasesCapture();
    MouseAxesThroughActualView();
    RawMotionFallbackAndCaptureWarp();
    QueuedCaptureMotion();
    if(failures)std::fprintf(stderr,"%d camera input checks failed\n",failures);
    else std::puts("PASS: camera input routing, capture, focus, movement, reset");
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
