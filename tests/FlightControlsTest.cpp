// AI-assisted CPU regressions for the mouse yoke and attitude control.
#include "aircraft/FlightControls.hpp"
#include "aircraft/Aircraft.hpp"
#include "aircraft/FlightProfile.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <glm/geometric.hpp>

using namespace flightcontrols;

namespace {
const double Pi=3.14159265358979323846;
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-4) {return std::abs(a-b)<tolerance;}
ControlInput In(float pitch,float roll) {ControlInput i;i.pitch=pitch;i.roll=roll;return i;}
double Heading(const glm::vec3& f) {return std::atan2(f.x,-f.z);}

Aircraft Level(float speed) {
    Aircraft a;
    a.set_forward(glm::vec3(0,0,-1)); // Heading north (-Z).
    a.set_up(glm::vec3(0,1,0));
    a.set_right(glm::vec3(1,0,0));
    a.set_velocity(glm::vec3(0,0,-speed));
    return a;
}
void Fly(Aircraft& a,ControlInput input,bool grounded,float seconds) {
    for(int i=0;i<int(seconds*60);++i)ApplyControls(a,input,grounded,1.0f/60.0f);
}

void Yoke() {
    auto center=MouseYoke(640,360,1280,720);
    Check(center.pitch==0&&center.roll==0,"window center is neutral");
    auto deadZone=MouseYoke(640+.04*640,360-.04*360,1280,720);
    Check(deadZone.pitch==0&&deadZone.roll==0,"small offsets fall inside the dead zone");
    auto down=MouseYoke(640,720,1280,720);
    Check(Near(down.pitch,1)&&down.roll==0,"mouse at the bottom edge is full nose up");
    auto up=MouseYoke(640,0,1280,720);
    Check(Near(up.pitch,-1),"mouse at the top edge is full nose down");
    auto right=MouseYoke(1280,360,1280,720);
    Check(Near(right.roll,1)&&right.pitch==0,"mouse at the right edge is full right roll");
    auto left=MouseYoke(0,360,1280,720);
    Check(Near(left.roll,-1),"mouse at the left edge is full left roll");
    auto outside=MouseYoke(5000,-5000,1280,720);
    Check(Near(outside.roll,1)&&Near(outside.pitch,-1),"positions outside the window are clamped");
    auto half=MouseYoke(960,360,1280,720);
    Check(half.roll>0&&half.roll<.5f,"response curve is finer near the center");
    auto invalid=MouseYoke(10,10,0,0);
    Check(invalid.pitch==0&&invalid.roll==0,"minimized window gives a neutral yoke");
}

void Rotation() {
    const auto r=RotateAroundAxis(glm::vec3(0,0,-1),glm::vec3(1,0,0),float(Pi/2));
    Check(Near(r.y,1)&&Near(r.z,0),"positive rotation about right raises the nose (Rodrigues)");
    Check(Near(PitchAngle(glm::normalize(glm::vec3(0,1,-1))),Pi/4),"pitch angle from forward");
    const glm::vec3 bankedUp=glm::normalize(glm::vec3(1,1,0));
    Check(Near(RollAngle(glm::vec3(0,0,-1),bankedUp),Pi/4),"up tilted to the right is positive roll");
}

// Ailerons only in flight: pitch is a load-factor demand in FlightModel.
void Roll() {
    const float rate=FlightProfileFor("a320").maxRollRate; // 15 deg/s
    auto a=Level(80);
    Fly(a,In(1,1),false,2);
    Check(Near(RollAngle(a.get_forward(),a.get_up()),2*rate,1e-2),"full right command rolls at the A320 roll rate");
    Check(Near(PitchAngle(a.get_forward()),0),"pitch command does not rotate the body here (FlightModel's job)");
    auto b=Level(80);
    Fly(b,In(0,-1),false,2);
    Check(RollAngle(b.get_forward(),b.get_up())<-.4f,"mouse left banks left");
    Fly(b,In(0,0),false,3);
    Check(Near(RollAngle(b.get_forward(),b.get_up()),-2*rate,1e-2),"A320 normal law: neutral holds the bank");
    // Unlimited roll: a full roll takes 2*pi/rate seconds.
    auto d=Level(80);
    float minUp=1;
    for(int i=0;i<int(2*Pi/rate*60)+1;++i) {
        ApplyControls(d,In(0,1),false,1.0f/60.0f);
        minUp=std::min(minUp,d.get_up().y);
    }
    Check(minUp<-.99f,"roll is not limited: the airplane flies inverted");
    Check(d.get_up().y>.99f,"holding full right completes a 360 degree roll");
    auto e=Level(80);
    e.set_up(glm::vec3(0,-1,0));
    Fly(e,In(0,0),false,3);
    Check(e.get_up().y<-.99f,"centered mouse keeps inverted flight");
    // The light aircraft has dihedral: wings slowly return to level.
    auto c=Level(50);c.set_profile(FlightProfileFor("light"));
    Fly(c,In(0,1),false,.5f);
    const float banked=RollAngle(c.get_forward(),c.get_up());
    Fly(c,In(0,0),false,3);
    Check(std::abs(RollAngle(c.get_forward(),c.get_up()))<banked*.5f,"light aircraft levels its wings");
    const auto f=c.get_forward(),u=c.get_up(),r=c.get_right();
    Check(Near(glm::length(f),1)&&Near(glm::length(u),1)&&Near(glm::dot(f,u),0)&&Near(glm::dot(r,f),0),
          "basis stays orthonormal");
}

void Ground() {
    auto a=Level(0);
    Fly(a,In(1,1),true,2);
    Check(Near(Heading(a.get_forward()),0)&&Near(PitchAngle(a.get_forward()),0),
          "stopped on the ground: no steering, no rotation");
    // Bicycle model: yaw rate = V tan(delta) / wheelbase, tiller at taxi speed.
    auto b=Level(3);
    ApplyControls(b,In(0,1),true,.1f);
    const float expected=3*std::tan(FlightProfileFor("a320").maxSteerLow)/12.64f*.1f;
    Check(Near(Heading(b.get_forward()),expected,1e-3),"taxi steering follows the wheelbase geometry");
    auto fast=Level(70);
    ApplyControls(fast,In(0,1),true,.1f);
    Check(Heading(fast.get_forward())<expected*10,"at takeoff speed only the small pedal angle remains");
    Check(Near(RollAngle(fast.get_forward(),fast.get_up()),0),"no roll on the ground");
    // Rotation: no elevator authority at low speed, ~3 deg/s at VR.
    auto slow=Level(20);
    Fly(slow,In(1,0),true,2);
    Check(PitchAngle(slow.get_forward())<.01f,"at 20 m/s the A320 cannot lift its nose");
    auto vr=Level(FlightProfileFor("a320").rotationSpeed);
    Fly(vr,In(1,0),true,2);
    Check(Near(PitchAngle(vr.get_forward()),2*FlightProfileFor("a320").rotationPitchRate,5e-3),
          "at VR full pull rotates at ~3 deg/s");
    auto c=Level(10);
    c.set_forward(glm::normalize(glm::vec3(0,.1f,-1)));c.set_up(glm::normalize(glm::vec3(0,1,.1f)));
    Fly(c,In(0,0),true,6);
    Check(std::abs(PitchAngle(c.get_forward()))<.01f,"after touchdown the nose lowers to the runway");
}
} // namespace

int main() {
    Yoke();Rotation();Roll();Ground();
    std::printf("flight_controls: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
