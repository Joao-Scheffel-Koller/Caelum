#include "camera/CameraController.hpp"
#include <algorithm>
#include <cmath>

namespace {
const float Pi=3.14159265359f;
const float Sensitivity=0.00060f; // Radians per mouse unit; double the previous 0.00030.
const glm::vec3 WorldUp(0.0f,1.0f,0.0f);

float Clamp(float value,float low,float high) {
    return std::max(low,std::min(value,high));
}
glm::vec3 Unit(const glm::vec3& vector,const glm::vec3& fallback) {
    const float lengthSquared=glm::dot(vector,vector);
    return std::isfinite(lengthSquared)&&lengthSquared>1e-10f
        ? vector/std::sqrt(lengthSquared) : fallback;
}
glm::vec3 Direction(float yaw,float pitch) {
    return glm::vec3(std::cos(pitch)*std::cos(yaw),std::sin(pitch),
                     std::cos(pitch)*std::sin(yaw));
}
glm::vec3 UprightRight(float yaw) {
    return glm::vec3(-std::sin(yaw),0.0f,std::cos(yaw));
}
void Orthonormalize(CameraPose& pose) {
    pose.forward=Unit(pose.forward,glm::vec3(1,0,0));
    glm::vec3 up=pose.up-pose.forward*glm::dot(pose.up,pose.forward);
    if(glm::dot(up,up)<1e-10f) {
        const glm::vec3 reference=std::abs(pose.forward.y)<.99f?WorldUp:glm::vec3(0,0,1);
        up=reference-pose.forward*glm::dot(reference,pose.forward);
    }
    pose.up=Unit(up,WorldUp);
}
CameraPose OrbitPose(const CameraFrame& frame,float yaw,float elevation,float distance) {
    // The orbit keeps the horizon level while following aircraft heading.
    glm::vec3 forward=glm::vec3(frame.aircraftModel[0]);
    forward.y=0;
    forward=Unit(forward,glm::vec3(1,0,0));
    const glm::vec3 right=glm::cross(forward,WorldUp);
    const glm::vec3 offset=(-forward*std::cos(yaw)+right*std::sin(yaw))*std::cos(elevation)
                          +WorldUp*std::sin(elevation);
    CameraPose pose;
    pose.position=frame.orbitTarget+distance*offset;
    pose.forward=Unit(frame.orbitTarget-pose.position,forward);
    pose.up=WorldUp;
    Orthonormalize(pose);
    return pose;
}
}

CameraType CameraController::Mode() const {return mode;}
float CameraController::OrbitDistance() const {return orbitDistance;}
float CameraController::FreeSpeed() const {return freeSpeed;}

void CameraController::CaptureFree(const CameraPose& source) {
    freePose=source;
    Orthonormalize(freePose);
    // Preserve the source's bank as well as its position and look direction.
    freeYaw=std::atan2(freePose.forward.z,freePose.forward.x);
    freePitch=std::asin(Clamp(freePose.forward.y,-1.0f,1.0f));
    const glm::vec3 right=UprightRight(freeYaw);
    const glm::vec3 up=Unit(glm::cross(right,freePose.forward),WorldUp);
    freeRoll=std::atan2(glm::dot(freePose.up,right),glm::dot(freePose.up,up));
    freePose.nearPlane=.5f;
}

void CameraController::SetMode(CameraType next,const CameraFrame& frame) {
    if(next==mode)return;
    if(next==CameraType::FREE)CaptureFree(Pose(frame));
    mode=next;
}

CameraPose CameraController::Pose(const CameraFrame& frame) const {
    if(mode==CameraType::FREE)return freePose;
    if(mode==CameraType::LOOK_AT)return OrbitPose(frame,orbitYaw,orbitElevation,orbitDistance);
    CameraPose pose;
    pose.position=glm::vec3(frame.aircraftModel*glm::vec4(frame.cockpitEyeLocal+headOffset,1));
    const glm::vec3 direction=Direction(headYaw,headPitch);
    const glm::vec3 localUp=glm::cross(UprightRight(headYaw),direction);
    pose.forward=glm::vec3(frame.aircraftModel*glm::vec4(direction,0));
    pose.up=glm::vec3(frame.aircraftModel*glm::vec4(localUp,0));
    pose.nearPlane=.05f;
    Orthonormalize(pose);
    return pose;
}

void CameraController::Look(float dx,float dy) {
    if(!std::isfinite(dx)||!std::isfinite(dy))return;
    // Wrapping yaw keeps long sessions from losing angular precision.
    if(mode==CameraType::LOOK_AT) {
        orbitYaw=std::remainder(orbitYaw+dx*Sensitivity,2*Pi);
        // In the external view, dragging upward raises the camera around the plane.
        orbitElevation=Clamp(orbitElevation-dy*Sensitivity,.02f,Pi/2-.01f);
    } else if(mode==CameraType::COCKPIT) {
        headYaw=Clamp(headYaw+dx*Sensitivity,-2*Pi/3,2*Pi/3);
        headPitch=Clamp(headPitch-dy*Sensitivity,-70*Pi/180,80*Pi/180);
    } else {
        freeYaw=std::remainder(freeYaw+dx*Sensitivity,2*Pi);
        freePitch=Clamp(freePitch-dy*Sensitivity,-Pi/2+.01f,Pi/2-.01f);
        freePose.forward=Direction(freeYaw,freePitch);
        const glm::vec3 right=UprightRight(freeYaw);
        const glm::vec3 up=glm::cross(right,freePose.forward);
        freePose.up=std::cos(freeRoll)*up+std::sin(freeRoll)*right;
        Orthonormalize(freePose);
    }
}

void CameraController::Scroll(float amount) {
    if(!std::isfinite(amount))return;
    amount=Clamp(amount,-100.0f,100.0f);
    if(mode==CameraType::LOOK_AT)
        orbitDistance=Clamp(orbitDistance*std::exp(-.1f*amount),5.0f,15000.0f);
    else if(mode==CameraType::FREE)
        freeSpeed=Clamp(freeSpeed*std::exp(.15f*amount),1.0f,2000.0f);
}

void CameraController::Update(const CameraMovement& movement,float dt,const CameraFrame&) {
    if(!std::isfinite(dt)||dt<=0)return;
    const glm::vec3 input(Clamp(movement.forward,-1,1),Clamp(movement.vertical,-1,1),Clamp(movement.right,-1,1));
    if(glm::dot(input,input)==0)return;
    if(mode==CameraType::COCKPIT) {
        headOffset+=Unit(input,glm::vec3(0))*.35f*dt;
        headOffset=glm::clamp(headOffset,glm::vec3(-.20f,-.12f,-.18f),glm::vec3(.20f,.12f,.18f));
    } else if(mode==CameraType::FREE) {
        const glm::vec3 right=Unit(glm::cross(freePose.forward,freePose.up),UprightRight(freeYaw));
        const glm::vec3 direction=freePose.forward*input.x+WorldUp*input.y+right*input.z;
        const float speed=freeSpeed*(movement.fast?5.0f:1.0f);
        freePose.position+=Unit(direction,glm::vec3(0))*speed*dt;
    }
}

void CameraController::Reset(const CameraFrame& frame) {
    const float distance=std::isfinite(frame.defaultOrbitDistance)?Clamp(frame.defaultOrbitDistance,5.0f,15000.0f):70.0f;
    if(mode==CameraType::COCKPIT) {
        headYaw=0;headPitch=0;headOffset=glm::vec3(0);
    } else if(mode==CameraType::LOOK_AT) {
        orbitYaw=0;orbitElevation=.25f;orbitDistance=distance;
    } else {
        CaptureFree(OrbitPose(frame,0,.25f,distance));
        freeSpeed=50;
    }
}
