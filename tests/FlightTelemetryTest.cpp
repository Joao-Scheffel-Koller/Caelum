// AI-assisted CPU regressions for flight instrumentation; no window or GPU.
#include "aircraft/FlightTelemetry.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-5) {return std::abs(a-b)<tolerance;}
FlightTelemetry Read(const glm::vec3& forward=glm::vec3(0,0,-1),
                     const glm::vec3& up=glm::vec3(0,1,0)) {
    return CalculateFlightTelemetry(glm::vec3(5000,120,6000),glm::vec3(3,4,12),forward,up,.42);
}
void Finite(const FlightTelemetry& t) {
    Check(std::isfinite(t.altitudeMeters)&&std::isfinite(t.speedKmh)&&
          std::isfinite(t.throttlePercent)&&std::isfinite(t.headingDegrees)&&
          std::isfinite(t.pitchDegrees)&&std::isfinite(t.rollDegrees),
          "all outputs stay finite even when a channel is invalid");
}
void Measurements() {
    const auto t=Read();
    Check(t.altitudeValid&&Near(t.altitudeMeters,120),"altitude uses world Y");
    Check(t.speedValid&&Near(t.speedKmh,46.8),"speed includes all axes and converts m/s to km/h");
    Check(t.throttleValid&&Near(t.throttlePercent,42),"throttle converts to percent");
    const glm::vec3 north(0,0,-1),up(0,1,0);
    const auto stopped=CalculateFlightTelemetry(glm::vec3(0,-15,0),glm::vec3(0),north,up,0);
    Check(stopped.altitudeValid&&Near(stopped.altitudeMeters,-15),"negative map altitude is preserved");
    Check(stopped.speedValid&&Near(stopped.speedKmh,0),"zero speed is valid");
    Check(Near(stopped.throttlePercent,0),"zero throttle is valid");
    const auto full=CalculateFlightTelemetry(glm::vec3(0),glm::vec3(0),north,up,1);
    Check(Near(full.throttlePercent,100),"full throttle");
    const auto below=CalculateFlightTelemetry(glm::vec3(0),glm::vec3(0),north,up,-4);
    const auto above=CalculateFlightTelemetry(glm::vec3(0),glm::vec3(0),north,up,7);
    Check(below.throttleValid&&Near(below.throttlePercent,0),"throttle lower clamp");
    Check(above.throttleValid&&Near(above.throttlePercent,100),"throttle upper clamp");
    const float large=std::numeric_limits<float>::max();
    const auto fast=CalculateFlightTelemetry(glm::vec3(0),glm::vec3(large),north,up,0);
    Check(fast.speedValid&&std::isfinite(fast.speedKmh),"double intermediate prevents float magnitude overflow");
}
void HeadingAndAttitude() {
    const glm::vec3 directions[]={{0,0,-1},{1,0,0},{0,0,1},{-1,0,0}};
    for(int i=0;i<4;++i) {
        const auto t=Read(directions[i]);
        Check(t.headingValid&&Near(t.headingDegrees,90*i),"cardinal heading convention");
        Check(t.attitudeValid&&Near(t.pitchDegrees,0)&&Near(t.rollDegrees,0),"level attitude independent of heading");
    }
    const auto wrapped=Read(glm::vec3(-.01f,0,-1));
    Check(wrapped.headingValid&&wrapped.headingDegrees>359&&wrapped.headingDegrees<360,
          "heading wraps through north into [0,360)");
    const auto zero=Read(glm::vec3(-0.0f,0,-1));
    Check(!std::signbit(zero.headingDegrees),"north does not display negative zero");
    const double radians=std::acos(-1.0)/180;
    for(double heading:{0.0,37.0,90.0,179.0,270.0})
    for(double pitch:{-70.0,-20.0,0.0,35.0,80.0})
    for(double roll:{-170.0,-45.0,0.0,25.0,170.0}) {
        const double h=heading*radians,p=pitch*radians,r=roll*radians;
        const glm::dvec3 front(std::sin(h)*std::cos(p),std::sin(p),-std::cos(h)*std::cos(p));
        const glm::dvec3 right(std::cos(h),0,std::sin(h));
        const glm::dvec3 levelUp=glm::cross(right,front);
        const glm::dvec3 bankedUp=levelUp*std::cos(r)+right*std::sin(r);
        // Add a forward component and scale to verify render-basis correction.
        const auto t=Read(glm::vec3(front*3.0),glm::vec3(bankedUp*2.0+front*.4));
        Check(t.headingValid&&t.attitudeValid,"synthetic 3D attitude remains valid");
        Check(Near(t.headingDegrees,heading,1e-4),"heading recovered from direction");
        Check(Near(t.pitchDegrees,pitch,1e-4),"pitch recovered from forward");
        Check(Near(t.rollDegrees,roll,1e-4),"roll recovered from corrected up");
    }
    const auto inverted=Read(glm::vec3(0,0,-1),glm::vec3(0,-1,0));
    Check(inverted.attitudeValid&&Near(std::abs(inverted.rollDegrees),180),"inverted flight is recognized");
    const double bank=30*radians;
    const auto rightBank=Read(glm::vec3(0,0,-1),glm::vec3(std::sin(bank),std::cos(bank),0));
    Check(Near(rightBank.rollDegrees,30),"positive bank lowers the right wing");
    for(float y:{-1.0f,1.0f}) {
        const auto vertical=Read(glm::vec3(0,y,0),glm::vec3(0,0,1));
        Check(!vertical.headingValid&&!vertical.attitudeValid,"vertical flight does not invent a heading or roll");
        Check(Near(vertical.pitchDegrees,y*90),"vertical pitch remains finite and descriptive");
        Finite(vertical);
    }
}
void InvalidAndReadonly() {
    const float nan=std::numeric_limits<float>::quiet_NaN();
    const float inf=std::numeric_limits<float>::infinity();
    const glm::vec3 zero(0),north(0,0,-1),up(0,1,0);
    for(float invalid:{nan,inf}) {
        const auto t=CalculateFlightTelemetry(glm::vec3(0,invalid,0),glm::vec3(invalid,1,0),north,up,invalid);
        Check(!t.altitudeValid&&!t.speedValid&&!t.throttleValid,"invalid measurements have independent flags");
        Check(t.headingValid&&t.attitudeValid,"invalid scalar channels do not invalidate orientation");
        Finite(t);
        const auto badForward=Read(glm::vec3(invalid,0,0));
        Check(!badForward.headingValid&&!badForward.attitudeValid,"nonfinite forward rejected");
        Finite(badForward);
        const auto badUp=Read(north,glm::vec3(0,invalid,0));
        Check(badUp.headingValid&&!badUp.attitudeValid,"up failure preserves direction-based heading");
        Finite(badUp);
    }
    const auto noForward=Read(zero);
    const auto noUp=Read(north,zero);
    const auto parallel=Read(north,north);
    Check(!noForward.headingValid&&!noForward.attitudeValid,"zero forward rejected");
    Check(noUp.headingValid&&!noUp.attitudeValid,"zero up rejected without hiding heading");
    Check(parallel.headingValid&&!parallel.attitudeValid,"parallel up rejected");
    Finite(noForward);Finite(noUp);Finite(parallel);
    glm::vec3 pos(1,2,3),velocity(4,5,6),front(.1f,.2f,-1),providedUp(.2f,1,.1f);
    const auto posBefore=pos,velocityBefore=velocity,frontBefore=front,upBefore=providedUp;
    CalculateFlightTelemetry(pos,velocity,front,providedUp,.5);
    Check(pos==posBefore&&velocity==velocityBefore&&front==frontBefore&&providedUp==upBefore,
          "telemetry never mutates aircraft inputs");
}
}
int main() {
    Measurements();HeadingAndAttitude();InvalidAndReadonly();
    std::printf("PASS: %d flight telemetry checks\n",checks);
}
