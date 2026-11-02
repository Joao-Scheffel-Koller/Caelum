// AI-assisted CPU tests of explicit OSM runway zones, independent of rendering.
#include "mission/Airports.hpp"
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
bool Near(double a,double b,double tolerance=1e-6) {return std::abs(a-b)<=tolerance;}
void RectangleCases() {
    const Runway north={"test","test",{0,0,0},{0,0,-100},20};
    Check(Contains(north,{0,0,-50}),"centerline lies inside north-south runway");
    Check(Contains(north,{10,200,-50}),"horizontal containment ignores finite altitude");
    Check(!Contains(north,{10.01,0,-50}),"point beyond the runway edge is outside");
    Check(Contains(north,{0,0,0})&&Contains(north,{0,0,-100}),"pavement endpoint boundaries are included");
    Check(!Contains(north,{0,0,.01})&&!Contains(north,{0,0,-100.01}),
          "runway end caps are rectangles rather than circles around endpoints");
    Check(Contains(north,{8,0,-2},2),"inset boundary reduces width and length equally");
    Check(!Contains(north,{8.01,0,-50},2)&&!Contains(north,{0,0,-1.99},2),
          "inset excludes both side strips and runway thresholds");
    Check(!Contains(north,{0,0,-50},10)&&!Contains(north,{0,0,-50},-1),
          "empty and negative-inset areas fail closed");
    Check(Near(DistanceToRunway(north,{0,100,-50}),0),"distance is horizontal and zero within the runway");
    Check(Near(DistanceToRunway(north,{13,0,-50}),3),"lateral distance measures the pavement edge");
    Check(Near(DistanceToRunway(north,{0,0,4}),4),"longitudinal distance measures the square end cap");
    Check(Near(DistanceToRunway(north,{13,0,4}),5),"corner distance follows the rectangle corner");
    const Runway diagonal={"diagonal","test",{1000,0,-2000},{1060,0,-1920},20};
    // Along=(.6,.8), right=(-.8,.6) in XZ. The point is 50 m along, 9 m across.
    Check(Contains(diagonal,{1022.8,0,-1954.6}),"oriented diagonal runway contains a near-edge point");
    Check(!Contains(diagonal,{1021.2,0,-1953.4}),"rotated point beyond the side is outside");
    Check(Near(DistanceToRunway(diagonal,{1021.2,0,-1953.4}),1),"distance rotates with the runway");
    const Runway reversed={"reversed","test",diagonal.end,diagonal.start,diagonal.width};
    Check(Contains(reversed,{1022.8,0,-1954.6})&&
          Near(DistanceToRunway(reversed,{1021.2,0,-1953.4}),1),"endpoint order does not change the runway area");
}
void RealMapCases() {
    Check(SantosDumont().id=="SBRJ"&&Galeao().id=="SBGL","airport identifiers match the original OSM extract");
    Check(SantosDumont().runways.size()==2&&Galeao().runways.size()==2,
          "displaced-threshold ways are not counted as additional runways");
    const auto& departure=SantosDumont().runways.front();
    Check(departure.id=="SBRJ-20L-02R"&&departure.width==42,"departure uses the actual 42 m OSM width");
    Check(Contains(departure,{5237.1933,0,6295.7127},2),"existing A320 spawn is inside the shrunken departure runway");
    Check(!Contains(SantosDumont().runways[1],{5237.1933,0,6295.7127}),"parallel airport runways remain distinct zones");
    for(const auto& airport:{SantosDumont(),Galeao()})for(const auto& runway:airport.runways) {
        const auto center=(runway.start+runway.end)*.5;
        Check(Contains(runway,center,2)&&Near(DistanceToRunway(runway,center),0),"all real runway centerpoints are within their own landing zone");
        Check(runway.width>=30&&runway.width<=47,"width is taken from the source OSM tag");
        const double length=std::hypot(runway.end.x-runway.start.x,runway.end.z-runway.start.z);
        Check(length>1200&&length<4100,"projected runway length is consistent with this airport map");
    }
    const auto& arrival=Galeao().runways.front();
    const double distance=DistanceToRunway(arrival,{5237.1933,0,6295.7127});
    Check(distance>11000&&distance<13000,"Santos Dumont to Galeao is measured in world meters, not model-local units");
    const double bearing=BearingToTarget({5237.1933,0,6295.7127},arrival.start);
    Check(bearing>310&&bearing<330,"actual destination is northwest of the departure runway");
    Check(arrival.start.x<0&&arrival.end.x<0,"Galeao runway uses real projected negative-X coordinates");
}
void BearingsAndInvalidCases() {
    Check(BearingToTarget({0,0,0},{0,0,-1})==0&&BearingToTarget({0,0,0},{1,0,0})==90,
          "north and east match the HUD convention");
    Check(BearingToTarget({0,0,0},{0,0,1})==180&&BearingToTarget({0,0,0},{-1,0,0})==270,
          "south and west match the HUD convention");
    Check(BearingToTarget({1,10,2},{1,-5,2})==0,"coincident horizontal target has a deterministic zero bearing");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    Runway broken={"bad","bad",{0,0,0},{0,0,0},20};
    Check(!Contains(broken,{0,0,0})&&std::isinf(DistanceToRunway(broken,{0,0,0})),"zero-length runway is never a landing zone");
    broken.end={0,0,-100};broken.width=-1;
    Check(!Contains(broken,{0,0,-50})&&std::isinf(DistanceToRunway(broken,{0,0,-50})),"invalid width fails closed");
    broken.width=20;
    Check(!Contains(broken,{nan,0,0})&&!Contains(broken,{0,0,-50},nan),"invalid positions/insets fail closed");
    Check(std::isinf(DistanceToRunway(broken,{0,nan,0})),"nonfinite world positions produce no finite distance claim");
    bool threw=false;try {BearingToTarget({0,0,0},{nan,0,0});}catch(const std::invalid_argument&) {threw=true;}
    Check(threw,"invalid bearing input is rejected explicitly");
}
}
int main() {
    RectangleCases();RealMapCases();BearingsAndInvalidCases();
    std::printf("PASS: %d explicit airport/runway geometry checks\n",checks);
}
