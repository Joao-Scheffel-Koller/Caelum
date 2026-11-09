// AI-assisted CPU regressions: the flight model reproduces published A320 and
// Cessna 172-class performance within gameplay tolerances. No window or GPU.
#include "aircraft/FlightModel.hpp"
#include "aircraft/FlightControls.hpp"
#include "aircraft/FlightProfile.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <glm/geometric.hpp>

using namespace flightcontrols;

namespace {
const float Knot=0.514444f,Degree=0.01745329f;
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
ControlInput In(float pitch,float roll) {ControlInput i;i.pitch=pitch;i.roll=roll;return i;}

Aircraft Make(const char* id,float speed,float altitude,int flaps) {
    Aircraft a;
    a.set_profile(FlightProfileFor(id));
    a.set_flap_index(flaps);
    a.set_forward(glm::vec3(0,0,-1));a.set_up(glm::vec3(0,1,0));a.set_right(glm::vec3(1,0,0));
    a.set_position(glm::vec3(0,altitude,0));
    a.set_velocity(glm::vec3(0,0,-speed));
    a.set_alpha(0);
    if(speed>1&&altitude>0) {
        // Start trimmed: alpha that gives lift = weight (CL = W / (q S)).
        const auto& p=a.get_profile();
        const float q=.5f*StandardAtmosphere(altitude).density*speed*speed;
        const float cl=p.mass*9.80665f/(q*p.wingArea);
        const float trim=(cl-p.CL0-a.get_flaps().deltaCL0)/p.CLalpha;
        a.set_alpha(trim);
        a.set_forward(glm::vec3(0,std::sin(trim),-std::cos(trim)));
        a.set_up(glm::vec3(0,std::cos(trim),std::sin(trim)));
    }
    return a;
}

// 1-g stall speed: V = sqrt(2 m g / (rho S CLmax)).
float StallSpeed(const FlightProfile& p,int flap) {
    return std::sqrt(2*p.mass*9.80665f/(1.225f*p.wingArea*p.flaps[flap].maxCL));
}

void Isa() {
    const auto sea=StandardAtmosphere(0),high=StandardAtmosphere(11000);
    Check(std::abs(sea.density-1.225f)<1e-4f&&std::abs(sea.soundSpeed-340.3f)<.2f,"ISA sea level");
    Check(std::abs(high.density-.3639f)<.003f&&std::abs(high.soundSpeed-295.1f)<.3f,"ISA 11 km");
}

void Coefficients() {
    const auto& p=FlightProfileFor("a320");
    const auto& clean=p.flaps[0];
    Check(std::abs(LiftCoefficient(p,clean,0)-p.CL0)<1e-6f,"CL(0)=CL0");
    const float stall=(clean.maxCL-p.CL0)/p.CLalpha;
    Check(stall>13*Degree&&stall<16*Degree,"clean A320 stalls at ~14-15 deg");
    Check(LiftCoefficient(p,clean,stall+5*Degree)<clean.maxCL,"lift falls beyond the stall");
    Check(DragCoefficient(p,clean,stall+20*Degree,.2f)>DragCoefficient(p,clean,stall,.2f),"stalled wing has more drag");
    Check(DragCoefficient(p,clean,0,.85f)>DragCoefficient(p,clean,0,.5f)+.005f,"transonic drag rise");
    const float ld=LiftCoefficient(p,clean,3*Degree)/DragCoefficient(p,clean,3*Degree,.3f);
    std::printf("  A320 L/D at 3 deg: %.1f\n",ld);
    Check(ld>14&&ld<19,"A320 lift-to-drag ratio ~17 (published max L/D 16-18)");
}

void StallSpeeds() {
    const auto& a=FlightProfileFor("a320");
    const float clean=StallSpeed(a,0)/Knot,takeoff=StallSpeed(a,1)/Knot,full=StallSpeed(a,4)/Knot;
    std::printf("  A320 60 t stall: clean %.0f kt, 1+F %.0f kt, FULL %.0f kt\n",clean,takeoff,full);
    Check(clean>135&&clean<155,"A320 clean stall ~145 kt");
    Check(takeoff>110&&takeoff<130,"A320 CONF 1+F stall ~120 kt");
    Check(full>95&&full<110,"A320 CONF FULL stall ~105 kt (Vref ~1.23 Vs ~ 130 kt)");
    const auto& c=FlightProfileFor("light");
    const float c0=StallSpeed(c,0)/Knot,c30=StallSpeed(c,3)/Knot;
    std::printf("  Light 1000 kg stall: clean %.0f kt, 30 deg %.0f kt\n",c0,c30);
    Check(c0>44&&c0<52&&c30>36&&c30<46,"Cessna 172 class: Vs 48 kt, Vso 40 kt");
}

void Thrust() {
    const auto& a=FlightProfileFor("a320");
    Check(std::abs(EngineThrust(a,1,0,0)-240200)<1,"two CFM56-5B4: 240 kN static");
    Check(EngineThrust(a,1,0,10000)<.5f*EngineThrust(a,1,0,0),"thrust falls with altitude");
    Check(EngineThrust(a,.5f,0,0)<EngineThrust(a,1,0,0),"throttle scales thrust");
    const auto& c=FlightProfileFor("light");
    Check(EngineThrust(c,1,60,0)<EngineThrust(c,1,10,0),"propeller thrust falls with speed (constant power)");
}

// Flat runway at Y=0 standing in for AircraftContacts.
struct Run {float distance=0,speed=0,pitch=0,time=0;bool liftoff=false;};
Run Takeoff(const char* id) {
    FlightModel model;
    auto a=Make(id,0,0,FlightProfileFor(id).takeoffFlap);
    a.set_throttle(1);
    const auto& p=a.get_profile();
    bool grounded=true;
    Run run;
    const float dt=1.0f/60.0f;
    for(int i=0;i<60*120&&!run.liftoff;++i) {
        const float speed=glm::length(a.get_velocity());
        // Pilot: rotate at VR to ~7.5 deg and hold (A320 SOP: ~3 deg/s rotation).
        const auto input=In(speed>=p.rotationSpeed&&PitchAngle(a.get_forward())<7.5f*Degree?1.0f:0.0f,0);
        ApplyControls(a,input,grounded,dt);
        model.update(&a,input,grounded,dt);
        auto pos=a.get_position();auto vel=a.get_velocity();
        if(pos.y<=0) {pos.y=0;vel.y=std::max(vel.y,0.0f);grounded=true;}
        else if(pos.y>.5f) {run.liftoff=true;}
        else {
            if(grounded&&vel.y>.1f)run.pitch=PitchAngle(a.get_forward()); // Wheels leave the runway.
            grounded=vel.y<=.1f;
        }
        a.set_position(pos);a.set_velocity(vel);
        run.distance=-pos.z;run.speed=glm::length(vel);run.time=i*dt;
    }
    return run;
}

void Takeoffs() {
    const auto a=Takeoff("a320");
    std::printf("  A320 takeoff: %.0f m, %.0f kt, pitch %.1f deg, %.0f s\n",a.distance,a.speed/Knot,a.pitch/Degree,a.time);
    Check(a.liftoff,"A320 lifts off");
    // Santos Dumont 20L: 1 323 m, spawn 100 m after the threshold.
    Check(a.distance>700&&a.distance<1200,"A320 at 60 t lifts off within Santos Dumont's runway");
    Check(a.speed>140*Knot&&a.speed<170*Knot,"A320 liftoff ~150-160 kt");
    Check(a.pitch<9*Degree,"rotation stays below the ~9 deg tail strike of the mesh");
    const auto c=Takeoff("light");
    std::printf("  Light takeoff: %.0f m, %.0f kt\n",c.distance,c.speed/Knot);
    Check(c.liftoff&&c.distance>150&&c.distance<450,"Cessna 172 class ground roll ~300 m");
}

struct Flight {float altitudeChange,headingChange,speed,maxAlpha;};
Flight Fly(Aircraft& a,ControlInput input,float seconds) {
    FlightModel model;
    const float y0=a.get_position().y;
    const auto f0=a.get_forward();
    float maxAlpha=-10;
    for(int i=0;i<int(seconds*60);++i) {
        ApplyControls(a,input,false,1.0f/60.0f);
        model.update(&a,input,false,1.0f/60.0f);
        maxAlpha=std::max(maxAlpha,a.get_alpha());
    }
    const auto f=a.get_forward();
    return {a.get_position().y-y0,std::atan2(f.x,-f.z)-std::atan2(f0.x,-f0.z),glm::length(a.get_velocity()),maxAlpha};
}

void FlightPath() {
    // Neutral sidestick holds the flight path (Airbus normal law).
    auto a=Make("a320",130,1000,0);a.set_throttle(.45f);
    const auto level=Fly(a,In(0,0),20);
    std::printf("  A320 level 20 s: dh %.1f m, speed %.0f kt, alpha %.1f deg\n",level.altitudeChange,level.speed/Knot,a.get_alpha()/Degree);
    Check(std::abs(level.altitudeChange)<15,"neutral stick holds level flight");
    Check(a.get_alpha()>0&&a.get_alpha()<8*Degree,"cruise angle of attack is a few degrees");
    // Pull: climbs; push: descends.
    auto up=Make("a320",130,1000,0);up.set_throttle(.45f);
    Check(Fly(up,In(.5f,0),5).altitudeChange>20,"mouse down climbs");
    auto down=Make("a320",130,1000,0);down.set_throttle(.45f);
    Check(Fly(down,In(-.5f,0),5).altitudeChange<-20,"mouse up descends");
    // 30 deg bank: coordinated turn at g tan(phi)/V, altitude kept.
    auto turn=Make("a320",130,1000,0);turn.set_throttle(.55f);
    Fly(turn,In(0,1),2);
    const float bank=RollAngle(turn.get_forward(),turn.get_up());
    const auto t=Fly(turn,In(0,0),10);
    const float expected=9.80665f*std::tan(bank)/130*10;
    std::printf("  A320 turn: bank %.0f deg, heading %.1f deg (theory %.1f), dh %.1f m\n",
                bank/Degree,t.headingChange/Degree,expected/Degree,t.altitudeChange);
    Check(t.headingChange>.7f*expected&&t.headingChange<1.3f*expected,"turn rate = g tan(bank)/V");
    Check(std::abs(t.altitudeChange)<30,"bank compensation keeps the altitude");
    // Alpha protection: full pull at low speed never stalls the wing.
    auto slow=Make("a320",90,1000,0);slow.set_throttle(0);
    const auto s=Fly(slow,In(1,0),20);
    const auto& p=slow.get_profile();
    Check(s.maxAlpha<=(p.flaps[0].maxCL-p.CL0)/p.CLalpha-p.alphaProtectionMargin+1e-4f,"alpha protection prevents the stall");
}

void TopSpeed() {
    // Full thrust, clean, FL350-ish ceiling of the ISA model (11 km).
    auto a=Make("a320",230,10500,0);a.set_throttle(1);
    const auto f=Fly(a,In(0,0),240);
    const float mach=f.speed/StandardAtmosphere(a.get_position().y).soundSpeed;
    std::printf("  A320 full thrust at %.0f m: Mach %.2f\n",a.get_position().y,mach);
    Check(mach>.72f&&mach<.85f,"thrust lapse and drag rise keep the A320 near its M0.78-0.82 cruise");
}

void Braking() {
    FlightModel model;
    auto a=Make("a320",60,0,4);
    a.set_brakes(true);
    float distance=0;
    for(int i=0;i<60*120&&glm::length(a.get_velocity())>.1f;++i) {
        model.update(&a,In(0,0),true,1.0f/60.0f);
        auto pos=a.get_position();auto vel=a.get_velocity();
        pos.y=0;vel.y=std::max(vel.y,0.0f);a.set_position(pos);a.set_velocity(vel);
        distance=-pos.z;
    }
    std::printf("  A320 braking from 117 kt: %.0f m\n",distance);
    Check(glm::length(a.get_velocity())<=.1f,"brakes stop the airplane");
    Check(distance>300&&distance<900,"landing roll with brakes, no reversers: ~500-800 m");
}
} // namespace

int main() {
    Isa();Coefficients();StallSpeeds();Thrust();Takeoffs();FlightPath();TopSpeed();Braking();
    std::printf("flight_model: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
