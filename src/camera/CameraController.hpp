#ifndef CAELUM_CAMERA_CONTROLLER_HPP
#define CAELUM_CAMERA_CONTROLLER_HPP

#include <glm/glm.hpp>

// AI-assisted camera math. This module has no window, renderer or physics state.
enum class CameraType { LOOK_AT, COCKPIT, FREE };

struct CameraFrame {
    glm::mat4 aircraftModel{1.0f};
    glm::vec3 orbitTarget{0.0f};
    glm::vec3 cockpitEyeLocal{7.90f,0.95f,-0.65f};
    float defaultOrbitDistance=70.0f;
};

struct CameraPose {
    glm::vec3 position{0.0f};
    glm::vec3 forward{1.0f,0.0f,0.0f};
    glm::vec3 up{0.0f,1.0f,0.0f};
    float nearPlane=0.5f;
    float farPlane=30000.0f;
    float fieldOfView=1.0471975512f; // Radians: 60 degrees.
};

struct CameraMovement {
    float forward=0.0f;
    float right=0.0f;
    float vertical=0.0f;
    bool fast=false;
};

class CameraController {
public:
    CameraType Mode() const;
    void SetMode(CameraType mode,const CameraFrame& frame);
    CameraPose Pose(const CameraFrame& frame) const;
    // Mouse displacement in pixels, positive X right and positive Y down.
    void Look(float dx,float dy);
    void Scroll(float amount);
    void Update(const CameraMovement& movement,float dt,const CameraFrame& frame);
    // Home: center the head, restore orbit, or put free view behind the aircraft.
    void Reset(const CameraFrame& frame);
    float OrbitDistance() const;
    float FreeSpeed() const;

private:
    CameraType mode=CameraType::LOOK_AT;
    float orbitYaw=0.0f,orbitElevation=0.25f,orbitDistance=70.0f;
    float headYaw=0.0f,headPitch=0.0f;
    glm::vec3 headOffset{0.0f};
    CameraPose freePose;
    float freeYaw=0.0f,freePitch=0.0f,freeRoll=0.0f,freeSpeed=50.0f;
    void CaptureFree(const CameraPose& pose);
};
#endif
