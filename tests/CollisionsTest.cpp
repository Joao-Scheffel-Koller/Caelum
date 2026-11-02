// AI-assisted regression tests for continuous CPU collision geometry.
#include "collision/collisions.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
using collision::Triangle;
using collision::Hit;
int checks=0;
void Check(bool condition,const char* message) {
    ++checks;
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-9) {return std::abs(a-b)<=tolerance;}
bool Near(const glm::dvec3& a,const glm::dvec3& b,double tolerance=1e-9) {
    return glm::length(a-b)<=tolerance;
}
void Valid(const Hit& hit,const char* message) {
    Check(hit.hit&&std::isfinite(hit.time)&&hit.time>=0&&hit.time<=1,message);
    Check(std::isfinite(hit.point.x)&&std::isfinite(hit.point.y)&&std::isfinite(hit.point.z),"contact point is finite");
    Check(std::isfinite(hit.normal.x)&&std::isfinite(hit.normal.y)&&std::isfinite(hit.normal.z)&&
          Near(glm::length(hit.normal),1),"contact normal is finite and normalized");
    Check(std::isfinite(hit.penetration)&&hit.penetration>=0,"penetration is finite and nonnegative");
}
const Triangle Floor={glm::dvec3(-100,0,-100),glm::dvec3(100,0,-100),glm::dvec3(0,0,100)};
const Triangle Corner={glm::dvec3(0,0,0),glm::dvec3(4,0,0),glm::dvec3(0,4,0)};

void FaceAndFastMotion() {
    auto hit=collision::SweepSphere({0,2,0},{0,-2,0},.5,Floor);
    Valid(hit,"falling sphere contacts floor");
    Check(Near(hit.time,.375)&&Near(hit.point,{0,0,0})&&Near(hit.normal,{0,1,0}),
          "floor contact uses sphere radius and preserves the upward impact normal");
    hit=collision::SweepSphere({0,-2,0},{0,2,0},.5,Floor);
    Valid(hit,"triangle is collidable from below as well");
    Check(Near(hit.time,.375)&&Near(hit.normal,{0,-1,0}),"backface normal points toward the incoming sphere");
    const Triangle reversed={Floor.c,Floor.b,Floor.a};
    hit=collision::SweepSphere({0,2,0},{0,-2,0},.5,reversed);
    Check(hit.hit&&Near(hit.time,.375)&&Near(hit.normal,{0,1,0}),"contact is independent of triangle winding");
    const Triangle wall={{0,-20,-20},{0,20,-20},{0,0,20}};
    hit=collision::SweepSphere({-5,0,0},{5,0,0},.25,wall); // 500 m/s over 20 ms.
    Valid(hit,"a 500m/s aircraft cannot tunnel through a thin wall");
    Check(Near(hit.time,.475)&&Near(hit.point,{0,0,0})&&Near(hit.normal,{-1,0,0}),
          "continuous wall impact occurs before the sphere center crosses the wall");
    Check(!collision::SweepSphere({0,2,0},{1,2,0},.5,Floor).hit,
          "parallel motion outside the floor never invents contact");
    Check(!collision::SweepSphere({0,2,0},{0,.500001,0},.5,Floor).hit,
          "a sweep ending just above the radius is not a collision");
    hit=collision::SweepSphere({0,2,0},{0,.5,0},.5,Floor);
    Check(hit.hit&&Near(hit.time,1),"contact exactly at the endpoint is included");
}
void EdgesAndVertices() {
    auto hit=collision::SweepSphere({2,-2,0},{2,2,0},.5,Corner);
    Valid(hit,"edge contact works while the center moves parallel to the face");
    Check(Near(hit.time,.375)&&Near(hit.point,{2,0,0})&&Near(hit.normal,{0,-1,0}),
          "edge cylinder produces the expected earliest contact");
    hit=collision::SweepSphere({2,-.5,-2},{2,-.5,2},.5,Corner);
    Valid(hit,"exact tangent against an edge is retained");
    Check(Near(hit.time,.5)&&Near(hit.point,{2,0,0}),"edge tangency has one root at mid trajectory");
    hit=collision::SweepSphere({-2,-2,0},{2,2,0},1,Corner);
    Valid(hit,"vertex sphere catches a corner impact");
    const double root=std::sqrt(.5);
    Check(Near(hit.time,(2-root)/4)&&Near(hit.point,{0,0,0})&&Near(hit.normal,{-root,-root,0}),
          "vertex impact has the expected radial normal");
    hit=collision::SweepSphere({-2,-1,0},{2,-1,0},1,Corner);
    Check(hit.hit&&Near(hit.time,.5),"vertex tangency is not discarded as no sign change");
    hit=collision::SweepSphere({-2,-.5,0},{6,-.5,0},.5,Corner);
    Check(hit.hit&&Near(hit.time,.25),"motion parallel to an edge enters through its endpoint cap");
    for(const Triangle& triangle:{Corner,Triangle{Corner.b,Corner.c,Corner.a},Triangle{Corner.c,Corner.a,Corner.b}}) {
        hit=collision::SweepSphere({2,-2,0},{2,2,0},.5,triangle);
        Check(hit.hit&&Near(hit.time,.375),"all cyclic edge positions are tested");
    }
}
void RestDegenerateAndPoints() {
    auto hit=collision::SweepSphere({0,.25,0},{0,.25,0},.5,Floor);
    Valid(hit,"stationary initial overlap is detected");
    Check(hit.time==0&&Near(hit.penetration,.25)&&Near(hit.normal,{0,1,0}),
          "initial overlap records actual penetration rather than a later exit");
    hit=collision::SweepSphere({0,.5,0},{0,3,0},.5,Floor);
    Check(hit.hit&&hit.time==0&&Near(hit.penetration,0),"initial touching is explicit even when moving away");
    Check(!collision::SweepSphere({0,2,0},{0,2,0},.5,Floor).hit,"stationary separated sphere remains separated");
    const Triangle line={{0,0,0},{4,0,0},{2,0,0}},point={{0,0,0},{0,0,0},{0,0,0}};
    hit=collision::SweepSphere({2,-2,0},{2,2,0},.5,line);
    Valid(hit,"collinear triangle falls back to edge geometry");
    Check(Near(hit.time,.375)&&Near(hit.point,{2,0,0}),"degenerate line has correct distance");
    hit=collision::SweepSphere({-2,0,0},{2,0,0},.5,point);
    Valid(hit,"zero-area point triangle still has a vertex");
    Check(Near(hit.time,.375)&&Near(hit.normal,{-1,0,0}),"point triangle uses a sphere sweep");
    hit=collision::SweepSphere({-2,1,0},{2,1,0},1,point);
    Check(hit.hit&&Near(hit.time,.5),"degenerate point has an exact tangent contact");
    hit=collision::SweepSphere({0,1,0},{0,-1,0},0,Floor);
    Valid(hit,"zero radius is a segment versus triangle query");
    Check(Near(hit.time,.5)&&Near(hit.normal,{0,1,0}),"zero-radius crossing normal opposes movement");
    hit=collision::SweepSphere({-2,0,0},{2,0,0},0,point);
    Valid(hit,"zero-radius segment can hit a degenerate point");
    Check(Near(hit.time,.5),"point crossing does not need nonzero radius");
    Check(Near(collision::ClosestPoint({1,1,3},Corner),{1,1,0}),"closest point projects onto face interior");
    Check(Near(collision::ClosestPoint({2,-3,1},Corner),{2,0,0}),"closest point outside the triangle lies on an edge");
    Check(Near(collision::ClosestPoint({-2,-3,1},Corner),{0,0,0}),"closest point can be a vertex");
    Check(Near(collision::ClosestPoint({2,4,0},line),{2,0,0}),"closest point handles collinear vertices");
}
void BoundsAndInvalidInput() {
    auto box=collision::Bounds(Corner);
    Check(Near(box.min,{0,0,0})&&Near(box.max,{4,4,0}),"triangle bounds include every vertex");
    const auto sweep=collision::SweptBounds({-5,2,3},{5,-2,-3},.5);
    Check(Near(sweep.min,{-5.5,-2.5,-3.5})&&Near(sweep.max,{5.5,2.5,3.5}),"swept bounds include the full path and radius");
    Check(collision::Overlaps(box,sweep),"broad phase sees a crossing even if endpoints are outside");
    collision::Aabb adjacent={{4,0,0},{5,5,5}};
    Check(collision::Overlaps(box,adjacent),"touching AABB faces count as overlap");
    adjacent.min.x=4.00001;
    Check(!collision::Overlaps(box,adjacent),"separated AABBs are rejected");
    adjacent.min.x=10;
    Check(!collision::Overlaps(box,adjacent),"malformed AABB never overlaps");
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    Check(!collision::SweepSphere({nan,0,0},{0,0,0},1,Corner).hit,"NaN sweep is rejected");
    Check(!collision::SweepSphere({0,0,0},{0,0,0},inf,Corner).hit,"infinite radius is rejected");
    Check(!collision::SweepSphere({0,0,0},{0,0,0},-1,Corner).hit,"negative radius is rejected");
    auto broken=Corner;broken.a.x=inf;
    Check(!collision::SweepSphere({0,0,0},{0,0,0},1,broken).hit,"nonfinite triangle is rejected");
    bool rejected=false;
    try {collision::Bounds(broken);} catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"bounds builder rejects nonfinite geometry");
    rejected=false;
    try {collision::SweptBounds({0,0,0},{0,0,0},-1);} catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"swept bounds builder rejects negative radius");
    rejected=false;
    try {collision::ClosestPoint({nan,0,0},Corner);} catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"closest point builder rejects a nonfinite query");
    const Triangle point={{0,0,0},{0,0,0},{0,0,0}};
    Check(!collision::SweepSphere({-1e8,1.0001,0},{1e8,1.0001,0},1,point).hit,
          "geometric verification rejects a cancellation-induced quadratic false positive");
}
glm::dvec3 Rotate(const glm::dvec3& p,double angle) {
    return {p.x,std::cos(angle)*p.y-std::sin(angle)*p.z,std::sin(angle)*p.y+std::cos(angle)*p.z};
}
void CoordinatesAndRandomRegression() {
    for(const glm::dvec3& offset:{glm::dvec3(5237,37,6295),glm::dvec3(-32700,0,-10000)})
        for(double angle:{0.0,.37,1.7,3.14}) {
            const Triangle triangle={Rotate(Floor.a,angle)+offset,Rotate(Floor.b,angle)+offset,Rotate(Floor.c,angle)+offset};
            const auto hit=collision::SweepSphere(Rotate({0,2,0},angle)+offset,Rotate({0,-2,0},angle)+offset,.5,triangle);
            Valid(hit,"translated and inclined triangles retain valid contacts");
            Check(Near(hit.time,.375,1e-8)&&Near(hit.normal,Rotate({0,1,0},angle),1e-8),
                  "georeferenced map coordinates do not alter time or orientation");
        }
    std::mt19937 random(0xCA310);
    std::uniform_real_distribution<double> position(-4,4),radius(.05,.8);
    for(int i=0;i<200;++i) {
        const glm::dvec3 from(position(random),position(random),position(random));
        const glm::dvec3 to(position(random),position(random),position(random));
        const double r=radius(random);
        const auto hit=collision::SweepSphere(from,to,r,Corner);
        bool sampledOverlap=false;
        for(int step=0;step<=400;++step) {
            const glm::dvec3 center=from+(to-from)*(step/400.0);
            if(glm::length(center-collision::ClosestPoint(center,Corner))<=r) {sampledOverlap=true;break;}
        }
        Check(!sampledOverlap||hit.hit,"continuous query never misses a sampled overlap");
        if(hit.hit) {
            Valid(hit,"random continuous contact is valid");
            const glm::dvec3 center=from+(to-from)*hit.time;
            Check(glm::length(center-hit.point)<=r+1e-8,"random contact lies within the physical radius");
            if(hit.time>1e-5) {
                const auto earlier=from+(to-from)*(hit.time-1e-5);
                Check(glm::length(earlier-collision::ClosestPoint(earlier,Corner))>=r-1e-8,
                      "random contact returns entry rather than an exit or later feature");
            }
        }
    }
}
}
int main() {
    FaceAndFastMotion();EdgesAndVertices();RestDegenerateAndPoints();BoundsAndInvalidInput();CoordinatesAndRandomRegression();
    std::printf("PASS: %d continuous collision geometry checks\n",checks);
}
