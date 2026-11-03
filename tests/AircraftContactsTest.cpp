// AI-assisted CPU regressions for wheel support, impact response and CCD.
#include "collision/AircraftContacts.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>

using namespace collision;
namespace {
int checks=0;
void Check(bool condition,const char* text) {
    ++checks;
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",text);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-5) {return std::abs(a-b)<=tolerance;}
AircraftPose Pose(double x=0,double y=0,double z=0) {
    AircraftPose p;p.position=glm::dvec3(x,y,z);return p;
}
AircraftPose FromResult(const ContactResult& r) {
    AircraftPose p;p.position=r.position;p.forward=r.forward;p.up=r.up;return p;
}
AircraftContacts Airplane() {
    AircraftContacts contacts;
    // Actual bounds of data/aircraft/aircraft.obj, recorded by a vertex survey.
    contacts.Configure({-27.425108,-3.342473,-17.178679},{10.548508,8.196299,17.178555});
    return contacts;
}
void Plane(CollisionWorld& world,Surface surface=Surface::Ground,double y=0,double half=2000) {
    const glm::vec3 a(-half,y,-half),b(half,y,-half),c(half,y,half),d(-half,y,half);
    world.AddChunk("plane",{{a,c,b,surface},{a,d,c,surface}});
}
void Wall(CollisionWorld& world,double x) {
    const glm::vec3 a(x,0,-100),b(x,100,-100),c(x,100,100),d(x,0,100);
    world.AddChunk("wall",{{a,b,c},{a,c,d}});
}
void Box(CollisionWorld& world,glm::vec3 low,glm::vec3 high) {
    glm::vec3 p[8];
    for(int i=0;i<8;++i)p[i]={i&1?high.x:low.x,i&2?high.y:low.y,i&4?high.z:low.z};
    std::vector<WorldTriangle> triangles;
    for(const auto& face:{glm::ivec4(0,1,3,2),glm::ivec4(4,6,7,5),glm::ivec4(0,4,5,1),
                         glm::ivec4(2,3,7,6),glm::ivec4(0,2,6,4),glm::ivec4(1,5,7,3)}) {
        triangles.emplace_back(p[face.x],p[face.y],p[face.z]);
        triangles.emplace_back(p[face.x],p[face.z],p[face.w]);
    }
    world.AddChunk("box",std::move(triangles));
}
void SupportAndTaxi() {
    CollisionWorld world;Plane(world);
    auto aircraft=Airplane();
    auto pose=Pose();
    auto result=aircraft.Resolve(world,pose,pose,{0,0,0},0);
    Check(result.state==AircraftState::Grounded,"level spawn is grounded instead of crashing");
    Check(result.gearContacts==3,"all three surveyed wheel supports touch level ground");
    Check(Near(result.position.y,0),"render pivot remains at the wheel-contact height");
    for(int i=0;i<120;++i) {
        pose=FromResult(result);
        auto target=pose;target.position.x+=.4;target.position.y-=.0003;
        result=aircraft.Resolve(world,pose,target,{24,-.018,0},1.0/60);
        Check(result.state==AircraftState::Grounded,"gravity during taxi cannot trigger a false body crash");
        Check(Near(result.position.x,(i+1)*.4)&&Near(result.position.y,0),"wheel support preserves horizontal taxi motion without vertical drift");
        Check(Near(result.velocity.x,24)&&Near(result.velocity.y,0),"ground cancels downward velocity without braking horizontal motion");
    }
    pose=FromResult(result);
    auto target=pose;target.position.y+=.1;
    result=aircraft.Resolve(world,pose,target,{24,6,0},1.0/60);
    Check(result.state==AircraftState::Flying&&Near(result.position.y,.1),"initial wheel tangency does not pin a taking-off aircraft");
    Check(result.velocity.y==6,"takeoff preserves upward velocity");
}
void Landings() {
    CollisionWorld world;Plane(world);
    auto soft=Airplane();
    auto landed=soft.Resolve(world,Pose(0,1),Pose(2,-.05),{4,-2,0},.5);
    Check(landed.state==AircraftState::Grounded,"gentle descending wheel contact is a landing");
    Check(Near(landed.position.x,2)&&Near(landed.position.y,0),"landing raises the predicted below-ground pivot while preserving travel");
    Check(landed.gearContacts==3&&landed.velocity.y==0,"landing supplies wheel contacts and cancels only sink speed");
    auto hard=Airplane();
    const auto crashed=hard.Resolve(world,Pose(0,4),Pose(2,-1),{4,-10,0},.5);
    Check(crashed.state==AircraftState::Crashed&&crashed.reason==CrashReason::HardLanding,"impact speed is checked before ground response zeros velocity");
    Check(crashed.position.y>=-1e-5&&crashed.position.x<2,"hard landing stops at the time of impact");
    Check(glm::length(crashed.velocity)==0,"crash freezes player translation");
    auto banked=Airplane();
    auto initial=Pose(),rolled=Pose();
    const double bank=12*std::acos(-1.0)/180;
    rolled.up={0,std::cos(bank),std::sin(bank)};
    const auto bad=banked.Resolve(world,initial,rolled,{0,0,0},1.0/60);
    if(bad.state!=AircraftState::Crashed||
       (bad.reason!=CrashReason::UnsafeAttitude&&bad.reason!=CrashReason::AirframeContact))
        std::fprintf(stderr,"bank debug state=%d reason=%d y=%.6f hitSurface=%d normalY=%.4f\n",int(bad.state),int(bad.reason),bad.position.y,int(bad.hit.surface),bad.hit.normal.y);
    Check(bad.state==AircraftState::Crashed&&
          (bad.reason==CrashReason::UnsafeAttitude||bad.reason==CrashReason::AirframeContact),
          "rolling past the landing bank limit at ground level causes an accident");
    auto inverted=Airplane();
    auto upside=Pose(0,10);upside.up={0,-1,0};
    auto upsideLow=upside;upsideLow.position.y=-2;
    Check(inverted.Resolve(world,upside,upsideLow,{0,-1,0},1).state==AircraftState::Crashed,
          "inverted body contact cannot be mistaken for a safe landing");
    auto elevated=Airplane();CollisionWorld higher;Plane(higher,Surface::Ground,50);
    Check(elevated.Resolve(higher,Pose(0,51),Pose(0,49.9),{0,-2,0},.5).state==AircraftState::Grounded,
          "contact uses actual map triangle elevation instead of a universal Y=0 plane");
}
void ObstaclesAndSweeps() {
    CollisionWorld world;Wall(world,100);
    auto fast=Airplane();
    const auto hit=fast.Resolve(world,Pose(0,20),Pose(2000,20),{40000,0,0},.05);
    Check(hit.state==AircraftState::Crashed&&hit.reason==CrashReason::Obstacle,"sphere CCD detects a wall crossed entirely within one frame");
    Check(hit.position.x>0&&hit.position.x<100,"high-speed aircraft stops before the nose passes through the wall");
    CollisionWorld side;
    Box(side,{-5,0,19},{5,30,22});
    auto wing=Airplane();
    const auto wingHit=wing.Resolve(side,Pose(0,10,0),Pose(0,10,6),{0,0,120},.05);
    Check(wingHit.state==AircraftState::Crashed&&wingHit.reason==CrashReason::Obstacle,
          "wing-volume contact is detected while the fuselage remains clear");
    CollisionWorld rotation;Box(rotation,{10,10,-14},{13,16,-10});
    auto rotating=Airplane();
    auto start=Pose(0,10),turned=start;turned.forward={0,0,-1};
    const auto turnHit=rotating.Resolve(rotation,start,turned,{0,0,0},.05);
    Check(turnHit.state==AircraftState::Crashed,"rotation substeps detect an obstacle along a nose arc");
    Check(turnHit.forward.x>.01&&turnHit.forward.z<-.01,
          "crash pose preserves the intermediate orientation at impact");
    CollisionWorld water;Plane(water,Surface::Water);
    auto swimmer=Airplane();
    const auto splash=swimmer.Resolve(water,Pose(0,1),Pose(0,-.05),{0,-1,0},.5);
    Check(splash.state==AircraftState::Crashed&&splash.reason==CrashReason::Water,
          "water contact is an accident even at a gentle sink speed");
    CollisionWorld taxiWall;Plane(taxiWall);Wall(taxiWall,60);
    auto taxi=Airplane();
    taxi.Resolve(taxiWall,Pose(),Pose(),{0,0,0},0);
    const auto atWall=taxi.Resolve(taxiWall,Pose(),Pose(100,0),{200,0,0},.5);
    Check(atWall.state==AircraftState::Crashed&&atWall.reason==CrashReason::Obstacle,
          "initial ground wheel contact cannot hide a later fuselage-wall impact during taxi");
}
void LifetimeAndGeometry() {
    CollisionWorld water;Plane(water,Surface::Water);
    auto aircraft=Airplane();
    const auto crash=aircraft.Resolve(water,Pose(0,1),Pose(0,-1),{0,-2,0},1);
    const auto held=aircraft.Resolve(water,Pose(100,100),Pose(200,100),{100,100,100},1);
    Check(held.state==AircraftState::Crashed&&glm::length(held.position-crash.position)<1e-8,
          "crash state remains frozen until reset");
    aircraft.Reset();CollisionWorld ground;Plane(ground);
    Check(aircraft.Resolve(ground,Pose(),Pose(),{0,0,0},0).state==AircraftState::Grounded,
          "reset clears the accident and restores normal wheel support");
    auto probes=aircraft.DebugSpheres(Pose());
    unsigned gears=0;
    for(const auto& sphere:probes) {
        Check(std::isfinite(sphere.center.x)&&std::isfinite(sphere.radius)&&sphere.radius>0,
              "debug and physical probes are finite positive volumes");
        if(sphere.gear) {++gears;Check(Near(sphere.center.y-sphere.radius,0),"gear spheres use exactly the rendering pivot contact plane");}
        else Check(sphere.center.y-sphere.radius>.1,"body/wing/engine proxies clear level ground at spawn");
    }
    Check(gears==3&&probes.size()>30,"collision model covers the airframe with three separate gear supports");
    const auto moved=aircraft.DebugSpheres(Pose(100,40,-50));
    for(size_t i=0;i<probes.size();++i)
        Check(glm::length(moved[i].center-probes[i].center-glm::dvec3(100,40,-50))<1e-8,
              "debug probes share the exact physical world transform");
    CollisionWorld empty;
    auto falling=Airplane();
    const auto unsupported=falling.Resolve(empty,Pose(0,1),Pose(0,-20),{0,-10,0},2);
    Check(unsupported.state==AircraftState::Flying&&unsupported.position.y==-20,
          "missing terrain does not invent a universal ground plane");
    auto invalid=Airplane();auto broken=Pose();broken.forward={0,0,0};
    const auto invalidResult=invalid.Resolve(empty,Pose(),broken,{0,0,0},.01);
    Check(invalidResult.state==AircraftState::Crashed&&invalidResult.reason==CrashReason::InvalidPose,
          "degenerate aircraft axes are rejected safely");
    bool rejected=false;
    try {invalid.Configure({0,0,0},{0,1,1});}catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"zero-length model bounds cannot create invalid collision spheres");
}
void WheelHazardsAfterSupport() {
    CollisionWorld taxiWorld;Plane(taxiWorld);
    Box(taxiWorld,{60,0,-30},{60.5f,.2f,30});
    auto taxi=Airplane();
    taxi.Resolve(taxiWorld,Pose(),Pose(),{0,0,0},0);
    const auto blocked=taxi.Resolve(taxiWorld,Pose(),Pose(100,0),{200,0,0},.5);
    Check(blocked.state==AircraftState::Crashed&&blocked.reason==CrashReason::Obstacle,
          "initial wheel support never hides a later low barrier touching only the landing gear");
    Check(blocked.position.x<60&&Near(blocked.position.y,0),
          "taxi wheel impact stops before the low obstacle at the actual supported height");
    auto departing=Airplane();departing.Resolve(taxiWorld,Pose(),Pose(),{0,0,0},0);
    const auto departure=departing.Resolve(taxiWorld,Pose(),Pose(100,.1),{200,.2,0},.5);
    Check(departure.state==AircraftState::Crashed&&departure.reason==CrashReason::Obstacle,
          "a departing wheel skips initial floor tangency without hiding a later low obstacle");

    auto landing=Airplane();
    const auto afterTouchdown=landing.Resolve(taxiWorld,Pose(0,1),Pose(100,-3),{50,-2,0},2);
    Check(afterTouchdown.state==AircraftState::Crashed&&afterTouchdown.reason==CrashReason::Obstacle,
          "a low obstacle after a gentle touchdown is found along the remaining ground segment");
    Check(afterTouchdown.position.x>25&&afterTouchdown.position.x<60&&Near(afterTouchdown.position.y,0),
          "post-touchdown impact uses piecewise landing/taxi motion rather than an airborne diagonal");

    CollisionWorld wet;Plane(wet);
    wet.AddChunk("water-patch",{{{60,0,-30},{70,0,-30},{70,0,30},Surface::Water},
                                {{60,0,-30},{70,0,30},{60,0,30},Surface::Water}});
    auto rolling=Airplane();rolling.Resolve(wet,Pose(),Pose(),{0,0,0},0);
    const auto splash=rolling.Resolve(wet,Pose(),Pose(100,0),{200,0,0},.5);
    Check(splash.state==AircraftState::Crashed&&splash.reason==CrashReason::Water,
          "a supported wheel entering water is detected even if it ends beyond the water patch");
}
void LightAircraftGeometry() {
    AircraftContacts light;light.Configure({-4.5,0,-5.5},{4.5,3,5.5},AirframeType::LightTaildragger);
    std::vector<ProbeSphere> gear;
    for(const auto& sphere:light.DebugSpheres(Pose()))if(sphere.gear)gear.push_back(sphere);
    Check(gear.size()==3,"light aircraft retains three distinct landing gear probes");
    Check(Near(gear[0].center.x,3.0759498)&&Near(gear[0].center.z,-1.6979006)&&
          Near(gear[1].center.z,1.6979006)&&Near(gear[2].center.x,-3.0598011),
          "taildragger support matches the converted mesh instead of the A320 nose gear");
    CollisionWorld floor;Plane(floor);
    auto result=light.Resolve(floor,Pose(),Pose(),{0,0,0},0);
    Check(result.state==AircraftState::Grounded&&result.gearContacts==3,
          "two main wheels and raised tail wheel support the light aircraft without a false body impact");
    result=light.Resolve(floor,Pose(),Pose(10,0),{5,0,0},2);
    Check(result.state==AircraftState::Grounded&&Near(result.position.x,10),
          "light aircraft can taxi with its own landing gear layout");
    CollisionWorld wingObstacle;Plane(wingObstacle);Box(wingObstacle,{.8,2.3,5.2},{1.5,2.7,5.5});
    light.Reset();result=light.Resolve(wingObstacle,Pose(),Pose(),{0,0,0},0);
    Check(result.state==AircraftState::Crashed&&result.reason==CrashReason::Obstacle,
          "high-wing proxy detects an obstacle at the light aircraft wing tip");
}
}
int main() {
    SupportAndTaxi();Landings();ObstaclesAndSweeps();LifetimeAndGeometry();WheelHazardsAfterSupport();
    LightAircraftGeometry();
    std::printf("PASS: %d aircraft support, impact and continuous collision checks\n",checks);
}
