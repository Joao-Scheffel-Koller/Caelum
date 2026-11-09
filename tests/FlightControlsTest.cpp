// AI-assisted CPU regressions for the mouse yoke and attitude control.
#include "aircraft/FlightControls.hpp"
#include "aircraft/Aircraft.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

void PitchControl() {
    auto a=Level(50);
    Fly(a,In(1,0),false,.5f);
    Check(PitchAngle(a.get_forward())>.2f,"mouse down pitches the nose up in flight");
    Check(a.get_velocity().y>0,"velocity follows the nose upward");
    Check(Near(glm::length(a.get_velocity()),50,1e-2),"alignment preserves speed");
    auto b=Level(50);
    Fly(b,In(-1,0),false,.5f);
    Check(PitchAngle(b.get_forward())<-.2f,"mouse up pitches the nose down");
    auto c=Level(50);
    Fly(c,In(1,0),false,10);
    Check(PitchAngle(c.get_forward())<=ControlTuning().maxPitch+.01f,"pitch is limited");
}

void RollAndTurn() {
    auto a=Level(50);
    Fly(a,In(0,1),false,.4f);
    Check(RollAngle(a.get_forward(),a.get_up())>.3f,"mouse right banks right");
    Fly(a,In(0,0),false,3);
    Check(Heading(a.get_forward())>.2,"a right bank turns the heading to the right");
    auto b=Level(50);
    Fly(b,In(0,-1),false,2);
    Check(RollAngle(b.get_forward(),b.get_up())<-.3f,"mouse left banks left");
    Check(Heading(b.get_forward())<-.2,"and turns left");
    Check(RollAngle(b.get_forward(),b.get_up())>=-ControlTuning().maxRoll-.02f,"roll is limited");
    auto c=Level(50);
    Fly(c,In(0,1),false,.4f);
    const float banked=RollAngle(c.get_forward(),c.get_up());
    Fly(c,In(0,0),false,3);
    Check(std::abs(RollAngle(c.get_forward(),c.get_up()))<banked,"centered mouse slowly levels the wings");
    const auto f=c.get_forward(),u=c.get_up(),r=c.get_right();
    Check(Near(glm::length(f),1)&&Near(glm::length(u),1)&&Near(glm::dot(f,u),0)&&Near(glm::dot(r,f),0),
          "basis stays orthonormal");
}

void Ground() {
    auto a=Level(0);
    Fly(a,In(1,1),true,2);
    Check(Near(Heading(a.get_forward()),0)&&Near(PitchAngle(a.get_forward()),0),
          "stopped on the ground: no steering, no rotation");
    auto b=Level(10);
    Fly(b,In(1,1),true,1);
    Check(Heading(b.get_forward())>.2,"rolling on the ground: lateral command steers the nose wheel");
    Check(Near(PitchAngle(b.get_forward()),0)&&Near(RollAngle(b.get_forward(),b.get_up()),0),
          "on the ground pitch and roll stay at the ground attitude");
    auto c=Level(10);
    c.set_forward(glm::normalize(glm::vec3(0,.1f,-1)));c.set_up(glm::normalize(glm::vec3(0,1,.1f)));
    Fly(c,In(0,0),true,4);
    Check(std::abs(PitchAngle(c.get_forward()))<.01f,"after touchdown the nose lowers to the runway");
}
} // namespace

int main() {
    Yoke();Rotation();PitchControl();RollAndTurn();Ground();
    std::printf("flight_controls: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
