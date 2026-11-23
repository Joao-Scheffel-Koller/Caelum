#include "aircraft/Propeller.hpp"
#include <algorithm>
#include <cmath>

// FONTE: codigo base do professor, include/matrices.h (definido em main.cpp).
// Forward declarations avoid duplicating the header's non-inline definitions.
glm::mat4 Matrix_Translate(float tx,float ty,float tz);
glm::mat4 Matrix_Rotate_X(float angle);

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
float BladeOpacity(float rpm) {return 1.0f-(1.0f-FaintBladeOpacity)*Blur(rpm);}
float DiscOpacity(float rpm) {return DiscMaxOpacity*Blur(rpm);}

glm::mat4 BladeMatrix(const glm::vec3& hub,float angle) {
    return Matrix_Translate(hub.x,hub.y,hub.z)*Matrix_Rotate_X(angle)*
           Matrix_Translate(-hub.x,-hub.y,-hub.z);
}
glm::mat4 DiscMatrix(const glm::vec3& hub) {return Matrix_Translate(hub.x,hub.y,hub.z);}
} // namespace propeller
