#include "aircraft/Propeller.hpp"
#include <algorithm>
#include <cmath>

// FONTE: codigo base do professor, include/matrices.h (definido em main.cpp).
// Forward declarations avoid duplicating the header's non-inline definitions.
glm::mat4 Matrix_Translate(float tx,float ty,float tz);
glm::mat4 Matrix_Scale(float sx,float sy,float sz);
glm::mat4 Matrix_Rotate_X(float angle);
glm::mat4 Matrix_Rotate_Z(float angle);

namespace propeller {
namespace {
const float TwoPi=6.28318531f;
float Clamp01(float v) {return std::max(0.0f,std::min(1.0f,v));}
// 0 at VisibleRpm, 1 at BlurRpm: how far the blades have turned into a disc.
float Blur(float rpm) {return Clamp01((rpm-VisibleRpm)/(BlurRpm-VisibleRpm));}
} // namespace

void Spin::Update(double dt,float throttle,float maxRpm) {
    if(!(dt>0))return;   // Paused, or a bad clock: nothing moves.
    // First-order engine response, exact for any step: the same speed after
    // the same time whatever the frame rate.
    const float target=Clamp01(throttle)*std::max(0.0f,maxRpm);
    const float before=rpm;
    rpm=target+(rpm-target)*static_cast<float>(std::exp(-dt/SpoolTime));
    // rpm -> rad/s of the rotation shown on screen. Trapezoidal rule (average
    // of the start and end of the frame): while the engine spools, long and
    // short frames give the same angle after the same time.
    const float shown=.5f*(DisplayedRpm(before)+DisplayedRpm(rpm));
    angle+=static_cast<float>(shown*TwoPi/60.0f*dt);
    angle=std::fmod(angle,TwoPi);
}

float DisplayedRpm(float rpm) {return std::min(std::max(0.0f,rpm),VisibleRpm);}
float BladeOpacity(float rpm,bool cockpit) {
    const float opacity=1.0f-(1.0f-FaintBladeOpacity)*Blur(rpm);
    return cockpit?std::min(opacity,CockpitBladeOpacity):opacity;
}
float DiscOpacity(float rpm,bool cockpit) {return (cockpit?CockpitDiscOpacity:DiscMaxOpacity)*Blur(rpm);}

float PlaneTilt(const Spec& spec) {
    // A plane's normal transforms with the inverse of the stretch.
    return std::atan2(std::sin(spec.shaftTilt)/spec.assetScale.y,
                      std::cos(spec.shaftTilt)/spec.assetScale.x);
}

namespace {
// OBJ -> hub at the origin, stretch undone, shaft along X, turned by angle.
glm::mat4 TurnedAboutX(const Spec& spec,float angle) {
    return Matrix_Rotate_X(angle)*Matrix_Rotate_Z(-spec.shaftTilt)*
           Matrix_Scale(1.0f/spec.assetScale.x,1.0f/spec.assetScale.y,1.0f/spec.assetScale.z)*
           Matrix_Translate(-spec.hub.x,-spec.hub.y,-spec.hub.z);
}
} // namespace

glm::mat4 BladeMatrix(const Spec& spec,float angle) {
    return Matrix_Translate(spec.hub.x,spec.hub.y,spec.hub.z)*
           Matrix_Scale(spec.bladeScale,spec.bladeScale,spec.bladeScale)*
           Matrix_Rotate_Z(PlaneTilt(spec))*TurnedAboutX(spec,angle);
}
glm::mat4 SpinnerMatrix(const Spec& spec,float angle) {
    return Matrix_Translate(spec.hub.x,spec.hub.y,spec.hub.z)*
           Matrix_Scale(spec.assetScale.x,spec.assetScale.y,spec.assetScale.z)*
           Matrix_Rotate_Z(spec.shaftTilt)*TurnedAboutX(spec,angle);
}
glm::mat4 DiscMatrix(const Spec& spec) {
    return Matrix_Translate(spec.hub.x,spec.hub.y,spec.hub.z)*Matrix_Rotate_Z(PlaneTilt(spec));
}
} // namespace propeller
