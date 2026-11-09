#include "aircraft/FlightControls.hpp"
#include "aircraft/Aircraft.hpp"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

namespace flightcontrols {
namespace {
const glm::vec3 WorldUp(0.0f,1.0f,0.0f);
const float HalfPi=1.57079632679f;
float Clamp(float v,float lo,float hi) {return std::max(lo,std::min(hi,v));}

bool Finite(const glm::vec3& v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}

// One axis of the yoke: [-1,1] after dead zone and response curve.
float Shape(double offset,const ControlTuning& tuning) {
    float value=static_cast<float>(std::max(-1.0,std::min(1.0,offset)));
    const float magnitude=std::fabs(value);
    if(magnitude<=tuning.deadZone)return 0.0f;
    // Rescale so the command starts at 0 right outside the dead zone.
    const float scaled=(magnitude-tuning.deadZone)/(1.0f-tuning.deadZone);
    return std::copysign(std::pow(scaled,tuning.responseExponent),value);
}

// Rotates the velocity direction toward the nose, keeping the speed.
glm::vec3 AlignVelocity(const glm::vec3& velocity,const glm::vec3& forward,float rate,float dt) {
    const float speed=glm::length(velocity);
    if(speed<1e-4f)return velocity;
    const float blend=1.0f-std::exp(-rate*dt);
    glm::vec3 direction=velocity/speed;
    // Only align when moving forward; never flip a backward roll on the ground.
    if(glm::dot(direction,forward)<=0.0f)return velocity;
    direction=glm::normalize(direction+(forward-direction)*blend);
    return direction*speed;
}
} // namespace

ControlInput MouseYoke(double x,double y,int width,int height,const ControlTuning& tuning) {
    ControlInput input;
    if(width<=0||height<=0||!std::isfinite(x)||!std::isfinite(y))return input;
    const double halfWidth=width*0.5,halfHeight=height*0.5;
    input.roll=Shape((x-halfWidth)/halfWidth,tuning);
    // GLFW's Y grows downward: mouse below the center pulls the nose up.
    input.pitch=Shape((y-halfHeight)/halfHeight,tuning);
    return input;
}

glm::vec3 RotateAroundAxis(const glm::vec3& v,const glm::vec3& axis,float angle) {
    // v' = v cos(a) + (k x v) sin(a) + k (k . v)(1 - cos(a))
    const float c=std::cos(angle),s=std::sin(angle);
    return v*c+glm::cross(axis,v)*s+axis*glm::dot(axis,v)*(1.0f-c);
}

float PitchAngle(const glm::vec3& forward) {
    return std::atan2(forward.y,std::hypot(forward.x,forward.z));
}

float RollAngle(const glm::vec3& forward,const glm::vec3& up) {
    const glm::vec3 levelRight=glm::cross(forward,WorldUp);
    if(glm::length(levelRight)<1e-6f)return 0.0f; // Vertical nose: roll undefined.
    const glm::vec3 right=glm::normalize(levelRight);
    const glm::vec3 levelUp=glm::cross(right,forward);
    return std::atan2(glm::dot(up,right),glm::dot(up,levelUp));
}

void ApplyControls(Aircraft& aircraft,const ControlInput& rawInput,bool grounded,float dt,
                   const ControlTuning& tuning) {
    if(!(dt>0.0f)||!std::isfinite(dt))return;
    glm::vec3 forward=aircraft.get_forward(),up=aircraft.get_up();
    if(!Finite(forward)||!Finite(up)||glm::length(forward)<1e-6f)return;
    forward=glm::normalize(forward);
    glm::vec3 right=glm::cross(forward,up);
    if(glm::length(right)<1e-6f)return;
    right=glm::normalize(right);
    up=glm::cross(right,forward);

    const FlightProfile& p=aircraft.get_profile();
    const float pitchCommand=Clamp(rawInput.pitch,-1.0f,1.0f);
    const float rollCommand=Clamp(rawInput.roll,-1.0f,1.0f);
    const float pitch=PitchAngle(forward);
    const float roll=RollAngle(forward,up);
    const glm::vec3 velocity=aircraft.get_velocity();
    const float rollingSpeed=std::max(0.0f,glm::dot(velocity,forward));

    float pitchRate=0.0f,rollRate=0.0f,yawRate=0.0f;
    if(grounded) {
        // Takeoff rotation: the elevator only overcomes the weight on the
        // nose wheel with enough dynamic pressure, ~ (V/VR)^2.
        const float ratio=rollingSpeed/p.rotationSpeed;
        const float authority=Clamp(ratio*ratio,0.0f,1.0f);
        // Pull: rotate. Push or no authority: the nose settles back on its
        // wheel. At VR a neutral command holds the rotation attitude.
        const float settle=pitchCommand<0.0f?1.0f:1.0f-authority;
        pitchRate=p.rotationPitchRate*authority*std::max(0.0f,pitchCommand)-tuning.groundLevelRate*settle*pitch;
        rollRate=-tuning.groundLevelRate*roll;
        // Kinematic (bicycle) steering: yaw rate = V tan(delta) / wheelbase.
        // Tiller angles at taxi speed blend into pedal angles at takeoff speed.
        const float fade=Clamp((rollingSpeed-p.steerFadeStart)/(p.steerFadeEnd-p.steerFadeStart),0.0f,1.0f);
        const float maxSteer=p.maxSteerLow+(p.maxSteerHigh-p.maxSteerLow)*fade;
        yawRate=rollingSpeed*std::tan(maxSteer*rollCommand)/p.wheelbase;
    } else {
        // Ailerons: roll-rate command. No attitude limits (loops, rolls and
        // inverted flight are allowed). Wing leveling, when the profile has
        // it, only acts while upright so it never fights inverted flight.
        rollRate=p.maxRollRate*rollCommand;
        if(rollCommand==0.0f&&std::fabs(roll)<HalfPi)rollRate-=p.rollStability*roll;
    }

    // Pitch about the body right axis (positive = nose up).
    forward=RotateAroundAxis(forward,right,pitchRate*dt);
    up=RotateAroundAxis(up,right,pitchRate*dt);
    // Roll about the body forward axis (positive = right wing down).
    up=RotateAroundAxis(up,forward,rollRate*dt);
    // Yaw about the world vertical (positive rate = heading to the right, so
    // the right-handed angle around +Y is negative).
    forward=RotateAroundAxis(forward,WorldUp,-yawRate*dt);
    up=RotateAroundAxis(up,WorldUp,-yawRate*dt);

    // Gram-Schmidt: keep the basis orthonormal despite float drift.
    forward=glm::normalize(forward);
    right=glm::normalize(glm::cross(forward,up));
    up=glm::cross(right,forward);
    aircraft.set_forward(forward);
    aircraft.set_up(up);
    aircraft.set_right(right);
    aircraft.set_pitch(PitchAngle(forward));
    aircraft.set_roll(RollAngle(forward,up));
    aircraft.set_yaw(std::atan2(forward.x,-forward.z));

    // Tyres: the velocity turns with the heading (no sideways skid).
    if(grounded)aircraft.set_velocity(AlignVelocity(velocity,forward,tuning.groundAlignment,dt));
}

} // namespace flightcontrols
