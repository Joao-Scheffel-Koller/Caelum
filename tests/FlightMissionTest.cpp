// AI-assisted CPU regressions for mission progression, independent of rendering.
#include "mission/FlightMission.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>

using namespace mission;
namespace {
int checks=0;
void Check(bool condition,const char* message) {
    ++checks;
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-8) {return std::abs(a-b)<tolerance;}
Airport Origin() {return {"A","Origem",{{"A1","01/19",{0,0,0},{0,0,-1000},40}}};}
Airport Destination() {return {"B","Destino",{
    {"B1","02/20",{0,0,-10000},{0,0,-12000},40},
    {"B2","09/27",{1000,0,-11000},{3000,0,-11000},40}}};}
MissionSample Sample(glm::dvec3 position={0,0,-100},bool grounded=true,double speed=0,
                     glm::dvec3 forward={0,0,-1}) {
    MissionSample sample;sample.position=position;sample.velocity=forward*speed;sample.forward=forward;
    sample.aircraftState=grounded?collision::AircraftState::Grounded:collision::AircraftState::Flying;
    sample.gearContacts=grounded?3:0;
    const glm::dvec3 right(-forward.z,0,forward.x);
    sample.gearCenters={position+forward*10.0+glm::dvec3(0,.25,0),
                        position-right*3.0+glm::dvec3(0,.25,0),
                        position+right*3.0+glm::dvec3(0,.25,0)};
    sample.gearRadii={.25,.25,.25};
    return sample;
}
void Depart(FlightMission& mission,int fps=60) {
    mission.Update(Sample(),0);
    Check(mission.Snapshot().originArmed,"ground support in the origin runway arms departure");
    mission.Update(Sample({0,5,-120},false),0);
    Check(!mission.Snapshot().departed,"first airborne sample cannot credit preceding ground time");
    for(int i=0;i<fps;++i)mission.Update(Sample({0,5,-120},false),1.0/fps);
    Check(mission.Snapshot().departed&&mission.Snapshot().stage==Stage::EnRoute,
          "qualified airborne time and height confirm a departure from the origin runway");
}
void DepartureHistory() {
    FlightMission mission(Origin(),Destination());
    auto atB=Sample({0,0,-10500});
    for(int i=0;i<180;++i)mission.Update(atB,1.0/60);
    Check(!mission.Snapshot().departed&&mission.Snapshot().stage==Stage::Departure,
          "spawning on the destination cannot complete a mission");
    mission.Reset();mission.Update(Sample(),0);
    mission.Update(Sample({0,0,-10500}),20);
    mission.Update(Sample({0,10,-10500},false),1);
    mission.Update(Sample({0,10,-10500},false),1);
    Check(!mission.Snapshot().departed&&!mission.Snapshot().originArmed,
          "taxiing outside the origin disarms departure rather than qualifying a flight at destination");
    mission.Reset();mission.Update(Sample(),0);
    mission.Update(Sample({0,10,-10500},false),0);
    mission.Update(Sample({0,10,-10500},false),1);
    Check(!mission.Snapshot().departed&&!mission.Snapshot().originArmed,
          "first airborne observation far from the origin cannot borrow its last ground contact");
    mission.Reset();mission.Update(Sample(),0);
    mission.Update(Sample({0,.4,-120},false),0);
    mission.Update(Sample({0,.4,-120},false),.2);
    mission.Update(Sample(),.2);
    Check(!mission.Snapshot().departed&&mission.Snapshot().originArmed,"a short bounce keeps the mission at departure");
    mission.Update(Sample({0,4.99,-120},false),0);
    mission.Update(Sample({0,4.99,-120},false),1);
    Check(!mission.Snapshot().departed,"airborne time alone cannot replace takeoff height");
    mission.Update(Sample({0,5,-120},false),0);
    Check(mission.Snapshot().departed,"reaching takeoff height after valid airborne time confirms flight");
    Check(mission.Snapshot().landingSeconds==0,"departure never starts the landing dwell timer");
    mission.Reset();auto bad=Sample();bad.gearContacts=1;
    mission.Update(bad,0);Check(!mission.Snapshot().originArmed,"one wheel cannot arm a departure");
    bad=Sample();bad.gearCenters.back().x=30;mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"origin arming checks every wheel against one explicit runway");
    bad=Sample({0,50,-100});mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"a high platform over the origin runway is not ground-level departure support");
}
void GuidanceAndApproach() {
    FlightMission mission(Origin(),Destination());Depart(mission);
    auto flight=Sample({0,100,-5000},false);mission.Update(flight,.1);
    const auto route=mission.Snapshot();
    Check(route.stage==Stage::EnRoute&&Near(route.target.z,-9000),"route targets the approach point before the first destination threshold");
    Check(Near(route.distanceMeters,4000)&&Near(route.bearingDegrees,0),"distance and bearing use the actual player and displayed target");
    mission.Update(Sample({0,100,-8001},false),.1);
    Check(mission.Snapshot().stage==Stage::Approach,"approach begins within two kilometers of a destination runway");
    mission.Update(Sample({0,100,-7600},false),.1);
    Check(mission.Snapshot().stage==Stage::Approach,"approach hysteresis prevents oscillation near the distance boundary");
    mission.Update(Sample({0,100,-7400},false),.1);
    Check(mission.Snapshot().stage==Stage::EnRoute,"leaving beyond two and a half kilometers returns to route guidance");
    mission.Update(Sample({0,500,-10500},false),50);
    Check(mission.Snapshot().stage!=Stage::Completed,"flying directly over the airport never wins by proximity");
    mission.Update(Sample({50,0,-10500}),.1);
    Check(mission.Snapshot().stage==Stage::Approach&&mission.Snapshot().landingSeconds==0,
          "ground beside the destination runway does not qualify as landing");
    Check(!mission.Snapshot().instruction.empty()&&!mission.Snapshot().targetName.empty(),"guidance has readable instruction and target labels");
}
void LandingGates() {
    FlightMission mission(Origin(),Destination());Depart(mission);
    auto landed=Sample({0,0,-10500},true,12);
    mission.Update(landed,20);
    Check(mission.Snapshot().stage==Stage::Landing&&mission.Snapshot().landingSeconds==0,
          "fast grounded arrival shows deceleration guidance without crediting preceding airborne time");
    landed.velocity={0,0,-3};mission.Update(landed,10);
    Check(mission.Snapshot().landingSeconds==0,"first slow sample does not borrow the previous high-speed interval");
    mission.Update(landed,1.25);Check(Near(mission.Snapshot().landingSeconds,1.25),"consecutive stable slow support accumulates elapsed time");
    auto off=landed;off.gearCenters[0].x=20;mission.Update(off,.1);
    Check(mission.Snapshot().landingSeconds==0&&mission.Snapshot().stage!=Stage::Landing,
          "a wheel outside the inset runway resets confirmation");
    mission.Update(landed,.1);mission.Update(landed,1);
    mission.Update(Sample({0,10,-10500},false),0);
    auto sideways=Sample({0,0,-10500},true,0,{1,0,0});mission.Update(sideways,.1);
    Check(mission.Snapshot().stage==Stage::Landing&&mission.Snapshot().landingSeconds==0,
          "a perpendicular ground arrival asks for alignment rather than completing");
    auto up=Sample({0,4,-10500},false);mission.Update(up,.1);
    Check(mission.Snapshot().landingSeconds==0,"taking off again clears landing confirmation");
    auto high=landed;high.position.y=50;for(auto& gear:high.gearCenters)gear.y=50.25;
    mission.Update(high,10);Check(mission.Snapshot().landingSeconds==0&&mission.Snapshot().stage!=Stage::Landing,
          "grounded state on an elevated roof over the runway does not win");
    auto missing=landed;missing.gearCenters.clear();mission.Update(missing,10);
    Check(mission.Snapshot().landingSeconds==0,"landing requires the surveyed three wheel positions");
    auto one=landed;one.gearContacts=1;mission.Update(one,10);
    Check(mission.Snapshot().landingSeconds==0,"one wheel is not stable landing support");
    auto opposite=Sample({0,0,-10500},true,0,{0,0,1});
    mission.Update(Sample({0,10,-10500},false,0,{0,0,1}),0);
    mission.Update(opposite,0);mission.Update(opposite,2);
    Check(mission.Snapshot().stage==Stage::Completed,"either runway direction can complete an aligned safe landing");
    mission.Reset();Depart(mission);
    auto other=Sample({1500,0,-11000},true,0,{1,0,0});mission.Update(other,0);mission.Update(other,2);
    Check(mission.Snapshot().stage==Stage::Completed,"the alternate explicitly modeled destination runway is accepted");
    mission.Reset();Depart(mission);mission.Update(landed,0);mission.Update(landed,1);
    mission.Update(other,1);
    Check(mission.Snapshot().landingSeconds==0,"landing confirmation cannot be carried between distinct destination runways");
    auto mixed=landed;mixed.gearCenters.back()=other.gearCenters.back();mission.Update(mixed,1);
    Check(mission.Snapshot().stage!=Stage::Landing,"all three wheel positions must fit the same runway rectangle");
}
void TouchdownHistory() {
    const auto landed=Sample({0,0,-10500});
    FlightMission mission(Origin(),Destination());Depart(mission);
    mission.Update(Sample({50,0,-10500}),.1);
    mission.Update(landed,.1);mission.Update(landed,5);
    Check(mission.Snapshot().stage!=Stage::Completed&&mission.Snapshot().landingSeconds==0,
          "landing on grass and taxiing onto a destination runway cannot complete the mission");

    mission.Reset();Depart(mission);mission.Update(Sample(),.1);
    mission.Update(landed,.1);mission.Update(landed,5);
    Check(mission.Snapshot().stage!=Stage::Completed&&mission.Snapshot().landingSeconds==0,
          "landing back at the origin and taxiing to the destination cannot replace destination touchdown");

    mission.Reset();Depart(mission);
    auto firstWheel=landed;firstWheel.gearContacts=1;
    mission.Update(firstWheel,.1);
    Check(mission.Snapshot().stage!=Stage::Completed,"one-wheel destination touchdown only arms an arrival candidate");
    auto settled=landed;settled.gearContacts=2;
    mission.Update(settled,.2);
    Check(mission.Snapshot().stage==Stage::Landing&&mission.Snapshot().landingSeconds==0,
          "two-wheel support after a valid first-wheel touchdown starts confirmation without borrowing its time");
    mission.Update(settled,1);
    mission.Update(Sample({0,0,-10505}),1);
    Check(mission.Snapshot().stage==Stage::Completed,
          "valid destination touchdown can continue rolling on the same runway while confirming a slow landing");

    mission.Reset();Depart(mission);mission.Update(landed,0);mission.Update(landed,1);
    mission.Update(Sample({50,0,-10500}),.1);
    mission.Update(landed,.1);mission.Update(landed,5);
    Check(mission.Snapshot().stage!=Stage::Completed&&mission.Snapshot().landingSeconds==0,
          "leaving a touched runway invalidates arrival and taxiing back cannot rearm it");
    mission.Update(Sample({0,10,-10500},false),.1);
    mission.Update(landed,0);mission.Update(landed,2);
    Check(mission.Snapshot().stage==Stage::Completed,"a new airborne approach and actual touchdown can recover an invalidated arrival");

    mission.Reset();Depart(mission);mission.Update(landed,0);mission.Update(landed,1);
    const auto other=Sample({1500,0,-11000},true,0,{1,0,0});
    mission.Update(other,.1);mission.Update(other,5);
    Check(mission.Snapshot().stage!=Stage::Completed&&mission.Snapshot().landingSeconds==0,
          "changing runways on the ground requires a new destination touchdown rather than restarting only its dwell timer");
    mission.Update(Sample({1500,10,-11000},false,0,{1,0,0}),.1);
    mission.Update(other,0);mission.Update(other,2);
    Check(mission.Snapshot().stage==Stage::Completed,"a new airborne touchdown on the alternate destination runway is valid");
}
void WheelHeightsAndRadii() {
    FlightMission mission(Origin(),Destination());Depart(mission);
    // A320 bounds and gear fractions match AircraftContacts::Configure.
    const double length=37.973616,span=34.357234;
    const double radius=length*.0065;
    const double pitch=-5.0*std::acos(-1.0)/180;
    const glm::dvec3 forward(0,std::sin(pitch),-std::cos(pitch));
    const glm::dvec3 up(0,std::cos(pitch),std::sin(pitch));
    const glm::dvec3 right(1,0,0);
    const double noseOffset=length*.37;
    const double pivotHeight=radius*(1-std::cos(pitch))-noseOffset*std::sin(pitch);
    auto noseFirst=Sample({0,pivotHeight,-10500},true,0,forward);
    noseFirst.gearContacts=1;
    noseFirst.gearRadii={radius,radius,radius};
    noseFirst.gearCenters={noseFirst.position+forward*noseOffset+up*radius,
        noseFirst.position+forward*(length*.025)+up*radius-right*(span*.095),
        noseFirst.position+forward*(length*.025)+up*radius+right*(span*.095)};
    Check(pivotHeight>1.2&&Near(noseFirst.gearCenters.front().y-radius,0),
          "real A320 five-degree nose-first contact places its pivot above the old one-meter limit");
    Check(noseFirst.gearCenters[1].y-radius>1&&noseFirst.gearCenters[2].y-radius>1,
          "nose-first regression has exactly one wheel at runway height");
    mission.Update(noseFirst,.1);
    Check(mission.Snapshot().stage==Stage::Approach&&mission.Snapshot().landingSeconds==0,
          "supported pitched first-wheel touchdown arms arrival without starting stable confirmation");
    const auto settled=Sample({0,0,-10500});
    mission.Update(settled,.2);
    Check(mission.Snapshot().stage==Stage::Landing&&mission.Snapshot().landingSeconds==0,
          "leveling after a valid high-pivot touchdown preserves the arrival candidate");
    mission.Update(settled,2);
    Check(mission.Snapshot().stage==Stage::Completed,
          "safe pitched first-wheel touchdown can finish after settling on the same runway");

    mission.Reset();Depart(mission);
    auto roof=noseFirst;
    roof.position.y+=50;for(auto& center:roof.gearCenters)center.y+=50;
    mission.Update(roof,.1);mission.Update(settled,0);mission.Update(settled,5);
    Check(mission.Snapshot().stage!=Stage::Completed&&mission.Snapshot().landingSeconds==0,
          "wheel heights reject roof support above a runway even if the pivot no longer gates touchdown");

    mission.Reset();auto oneAtHeight=Sample();
    oneAtHeight.gearCenters[1].y+=2;oneAtHeight.gearCenters[2].y+=2;
    mission.Update(oneAtHeight,0);
    Check(!mission.Snapshot().originArmed,"claimed support without two wheels at runway height cannot arm departure");
    oneAtHeight.gearCenters[1].y-=2;mission.Update(oneAtHeight,0);
    Check(mission.Snapshot().originArmed,"two physically supported wheel bottoms at runway height arm departure");

    for(double invalidRadius:{0.0,-.25,std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::infinity()}) {
        mission.Reset();auto bad=Sample();bad.gearRadii[1]=invalidRadius;mission.Update(bad,0);
        Check(!mission.Snapshot().originArmed,"nonpositive or nonfinite wheel radii fail closed");
    }
    mission.Reset();auto bad=Sample();bad.gearRadii.pop_back();mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"wheel centers and radii must have matching counts");
    mission.Reset();bad=Sample();bad.gearRadii.clear();mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"missing wheel radii cannot arm departure");
    mission.Reset();bad=Sample();bad.gearCenters.pop_back();bad.gearRadii.pop_back();mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"partial wheel surveys cannot arm departure");
    mission.Reset();bad=Sample();bad.gearCenters.clear();bad.gearRadii.clear();mission.Update(bad,0);
    Check(!mission.Snapshot().originArmed,"grounded telemetry requires all three wheel spheres");

    mission.Reset();Depart(mission);auto emptyFlying=Sample({0,100,-8001},false);
    emptyFlying.gearCenters.clear();emptyFlying.gearRadii.clear();mission.Update(emptyFlying,.1);
    Check(mission.Snapshot().stage==Stage::Approach,
          "airborne navigation accepts an empty matching wheel survey without assuming ground support");
    auto partialFlying=emptyFlying;partialFlying.gearCenters={{0,100,-8001}};
    partialFlying.gearRadii={.25};mission.Update(partialFlying,.1);
    Check(mission.Snapshot().stage==Stage::EnRoute&&mission.Snapshot().instruction.find("invalida")!=std::string::npos,
          "airborne telemetry still rejects a partial nonempty wheel survey");
}
void LifetimeAndInvalidData() {
    FlightMission mission(Origin(),Destination());Depart(mission);
    auto landed=Sample({0,0,-10500});mission.Update(landed,0);mission.Update(landed,1.99);
    auto crashed=landed;crashed.aircraftState=collision::AircraftState::Crashed;
    mission.Update(crashed,.1);
    Check(mission.Snapshot().stage==Stage::Failed,"accident has priority over a landing timer expiring in the same frame");
    mission.Update(landed,100);Check(mission.Snapshot().stage==Stage::Failed,"failed mission is latched until explicit reset");
    mission.Reset();Check(mission.Snapshot().stage==Stage::Departure&&!mission.Snapshot().departed&&
          !mission.Snapshot().originArmed&&mission.Snapshot().flightSeconds==0&&mission.Snapshot().landingSeconds==0,
          "reset clears all mission history and clocks");
    Depart(mission);mission.Update(landed,0);mission.Update(landed,2);
    const auto won=mission.Snapshot();mission.Update(crashed,20);
    Check(mission.Snapshot().stage==Stage::Completed&&mission.Snapshot().flightSeconds==won.flightSeconds,
          "completed mission and elapsed result remain latched until reset");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(double dt:{-1.0,nan,std::numeric_limits<double>::infinity()}) {
        mission.Reset();Depart(mission);mission.Update(landed,0);mission.Update(landed,1);
        mission.Update(landed,dt);
        Check(mission.Snapshot().landingSeconds==0&&mission.Snapshot().stage!=Stage::Completed,
              "invalid elapsed time cannot advance or preserve a stale landing timer");
    }
    mission.Reset();Depart(mission);mission.Update(landed,0);mission.Update(landed,1);
    auto broken=landed;broken.velocity.x=nan;mission.Update(broken,.1);
    Check(mission.Snapshot().landingSeconds==0,"nonfinite velocity clears landing confirmation");
    mission.Update(landed,10);Check(mission.Snapshot().landingSeconds==0,"first recovered valid sample cannot credit an unobserved interval");
    auto wrong=landed;wrong.forward={0,0,0};mission.Update(wrong,10);
    Check(mission.Snapshot().stage!=Stage::Completed,"degenerate heading cannot qualify as a landing");
    Check(std::isfinite(mission.Snapshot().bearingDegrees)&&std::isfinite(mission.Snapshot().distanceMeters),
          "invalid input never puts nonfinite guidance on the HUD");
    bool rejected=false;
    try {FlightMission bad(Airport{"empty","empty",{}},Destination());}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"mission construction rejects airports without runways");
}
void FrameRatesAndRealData() {
    double times[3];int index=0;
    for(int fps:{30,60,144}) {
        FlightMission mission(Origin(),Destination());Depart(mission,fps);
        const auto landed=Sample({0,0,-10500});mission.Update(landed,0);
        for(int frame=0;frame<2*fps;++frame)mission.Update(landed,1.0/fps);
        Check(mission.Snapshot().stage==Stage::Completed&&Near(mission.Snapshot().landingSeconds,2),
              "two-second landing succeeds equally at 30, 60 and 144 FPS");
        times[index++]=mission.Snapshot().flightSeconds;
    }
    Check(Near(times[0],times[1])&&Near(times[1],times[2]),"mission clock is independent of the update frequency");
    FlightMission real;
    const auto& origin=SantosDumont().runways.front();
    const auto forward=(origin.end-origin.start)/glm::length(origin.end-origin.start);
    auto start=Sample(origin.start+forward*100.0,true,0,forward);
    real.Update(start,0);Check(real.Snapshot().originArmed,"real Santos Dumont spawn arms the production mission");
    auto flying=start;flying.aircraftState=collision::AircraftState::Flying;flying.gearContacts=0;flying.position.y=8;
    real.Update(flying,0);real.Update(flying,.5);
    Check(real.Snapshot().departed,"real source runway metadata supports confirmed departure");
    const auto& target=Galeao().runways.front();
    const auto direction=(target.end-target.start)/glm::length(target.end-target.start);
    auto end=Sample(target.start*.5+target.end*.5,true,0,direction);
    real.Update(end,0);real.Update(end,2);
    Check(real.Snapshot().stage==Stage::Completed,"production Galeao rectangle accepts a confirmed safe arrival");
}
}
int main() {
    DepartureHistory();GuidanceAndApproach();LandingGates();TouchdownHistory();WheelHeightsAndRadii();LifetimeAndInvalidData();FrameRatesAndRealData();
    std::printf("PASS: %d flight mission progression and safety checks\n",checks);
}
