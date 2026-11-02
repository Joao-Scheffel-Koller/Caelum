// AI-assisted CPU regressions for visual needle response; no graphics or physics.
#include "ui/HudNeedleMotion.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

namespace {
int checks=0;
void Check(bool condition,const char* message) {
    ++checks;
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-9) {return std::abs(a-b)<tolerance;}
FlightTelemetry Sample(double speed=0,double throttle=0,double altitude=999,double heading=359) {
    FlightTelemetry t;
    t.speedKmh=speed;t.throttlePercent=throttle;t.altitudeMeters=altitude;t.headingDegrees=heading;
    t.speedValid=t.throttleValid=t.altitudeValid=t.headingValid=t.attitudeValid=true;
    return t;
}
void Finite(const FlightTelemetry& t) {
    Check(std::isfinite(t.speedKmh)&&std::isfinite(t.throttlePercent)&&
          std::isfinite(t.altitudeMeters)&&std::isfinite(t.headingDegrees)&&
          std::isfinite(t.pitchDegrees)&&std::isfinite(t.rollDegrees),"all displayed fields remain finite");
}
FlightTelemetry AfterOneSecond(int frames) {
    HudNeedleMotion motion;
    motion.Update(Sample(),0);
    const auto target=Sample(1000,100,1001,1);
    for(int i=0;i<frames;++i)motion.Update(target,1.0/frames);
    return motion.Update(target,0);
}
void FrameRates() {
    const auto a=AfterOneSecond(30),b=AfterOneSecond(60),c=AfterOneSecond(144);
    for(const auto& reading:{b,c}) {
        Check(Near(reading.speedKmh,a.speedKmh)&&Near(reading.throttlePercent,a.throttlePercent),
              "one-second scalar response is equal at 30, 60 and 144 FPS");
        Check(Near(reading.altitudeMeters,a.altitudeMeters)&&Near(reading.headingDegrees,a.headingDegrees),
              "one-second circular response is equal at 30, 60 and 144 FPS");
    }
    Check(a.speedKmh>999&&a.speedKmh<1000,"needle approaches its target without overshooting");
    Check(a.headingDegrees>0&&a.headingDegrees<1,"north crossing approaches the requested side of north");
}
void StepsAndWraps() {
    HudNeedleMotion motion;
    Check(Near(motion.Update(Sample(),0).headingDegrees,359),"first valid sample appears immediately");
    auto target=Sample(1000,5,1001,1);
    target.pitchDegrees=20;target.rollDegrees=-30;
    const auto intermediate=motion.Update(target,1.0/60);
    Check(intermediate.throttlePercent>0&&intermediate.throttlePercent<5,
          "five-percent throttle command produces intermediate visual positions");
    Check(intermediate.speedKmh>0&&intermediate.speedKmh<1000,
          "speed needle moves along the value scale instead of across the dial gap");
    Check(intermediate.altitudeMeters>999&&intermediate.altitudeMeters<1000,
          "ascending altitude is filtered in meters before the thousand-meter wrap");
    Check(intermediate.headingDegrees>359&&intermediate.headingDegrees<360,
          "359-to-1 heading takes the short clockwise arc");
    Check(intermediate.pitchDegrees==20&&intermediate.rollDegrees==-30,
          "pitch and roll remain immediate real readings");
    double previous=intermediate.speedKmh;
    for(int i=0;i<60;++i) {
        const auto& displayed=motion.Update(target,1.0/60);
        Check(displayed.speedKmh>=previous&&displayed.speedKmh<=1000,"rising speed is monotonic without overshoot");
        previous=displayed.speedKmh;
    }
    HudNeedleMotion reverse;
    reverse.Update(Sample(0,0,1001,1),0);
    const auto descending=reverse.Update(Sample(0,0,999,359),1.0/60);
    Check(descending.headingDegrees>0&&descending.headingDegrees<1,
          "1-to-359 heading takes the short counterclockwise arc");
    Check(descending.altitudeMeters<1001&&descending.altitudeMeters>1000,
          "descending altitude remains continuous across the thousand-meter boundary");
    Check(target.speedKmh==1000&&target.throttlePercent==5&&target.altitudeMeters==1001&&
          target.headingDegrees==1&&target.pitchDegrees==20&&target.rollDegrees==-30,
          "target telemetry is never changed by visual filtering");
}
void TimeAndValidity() {
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double infinity=std::numeric_limits<double>::infinity();
    for(double dt:{0.0,-1.0,nan,infinity}) {
        HudNeedleMotion motion;
        motion.Update(Sample(10,20,30,40),dt);
        auto target=Sample(100,50,300,80);
        target.pitchDegrees=7;target.rollDegrees=9;
        const auto frozen=motion.Update(target,dt);
        Check(frozen.speedKmh==10&&frozen.throttlePercent==20&&frozen.altitudeMeters==30&&frozen.headingDegrees==40,
              "invalid or nonpositive elapsed time freezes existing valid channels");
        Check(frozen.pitchDegrees==7&&frozen.rollDegrees==9,"attitude is not delayed by frozen needle time");
        const auto invalid=motion.Update(FlightTelemetry{},dt);
        Check(!invalid.speedValid&&!invalid.throttleValid&&!invalid.altitudeValid&&!invalid.headingValid&&!invalid.attitudeValid,
              "invalid flags clear channels even while time is frozen");
        Finite(invalid);
        const auto restored=motion.Update(target,dt);
        Check(restored.speedKmh==100&&restored.throttlePercent==50&&restored.altitudeMeters==300&&restored.headingDegrees==80,
              "reactivated channels initialize directly even while time is frozen");
    }
    HudNeedleMotion motion;
    motion.Update(Sample(),0);
    auto broken=Sample(nan,infinity,nan,infinity);
    broken.pitchDegrees=nan;broken.rollDegrees=infinity;
    const auto rejected=motion.Update(broken,1.0/60);
    Check(!rejected.speedValid&&!rejected.throttleValid&&!rejected.altitudeValid&&!rejected.headingValid&&!rejected.attitudeValid,
          "nonfinite readings invalidate channels even when their input flags claim validity");
    Finite(rejected);
    const auto restored=motion.Update(Sample(12,34,-20,-1),0);
    Check(restored.speedValid&&restored.throttleValid&&restored.altitudeValid&&restored.headingValid&&
          restored.speedKmh==12&&restored.throttlePercent==34&&restored.altitudeMeters==-20&&restored.headingDegrees==359,
          "recovery preserves signed altitude and normalizes heading");
    auto partiallyBroken=Sample(50,60,70,80);
    partiallyBroken.speedKmh=nan;
    const auto partial=motion.Update(partiallyBroken,1.0/60);
    Check(!partial.speedValid&&partial.throttleValid&&partial.altitudeValid&&partial.headingValid&&partial.attitudeValid,
          "a failed sensor does not invalidate unrelated channels");
    const double large=std::numeric_limits<double>::max();
    HudNeedleMotion extremes;
    extremes.Update(Sample(large,large,-large,large),0);
    const auto extreme=extremes.Update(Sample(-large,-large,large,-large),.1);
    Finite(extreme);
}
}
int main() {
    FrameRates();StepsAndWraps();TimeAndValidity();
    std::printf("PASS: %d continuous HUD needle checks\n",checks);
}
