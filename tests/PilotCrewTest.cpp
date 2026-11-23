// AI-assisted regressions for the seated pilots (docs/pilot.md).
#include "aircraft/PilotCrew.hpp"
#include "aircraft/AircraftCatalog.hpp"
#include "matrices.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(const glm::vec3& a,const glm::vec3& b,float tolerance=1e-4f) {
    return glm::length(a-b)<tolerance;
}
glm::vec3 Point(const glm::mat4& m,const glm::vec3& p) {return glm::vec3(m*glm::vec4(p,1.0f));}
glm::vec3 Direction(const glm::mat4& m,const glm::vec3& d) {return glm::vec3(m*glm::vec4(d,0.0f));}
const AircraftSpec& Spec(const char* id) {
    for(const auto& spec:AircraftCatalog())if(spec.id==id)return spec;
    std::fprintf(stderr,"FAIL: aircraft %s missing\n",id);std::exit(EXIT_FAILURE);
}
} // namespace

int main() {
    // The seat matrix: anchor on the seat, front to the nose, up stays up.
    const glm::vec3 seat(2.0f,1.695f,-.30f);
    const glm::mat4 m=pilot::SeatMatrix(seat,.8f);
    Check(Near(Point(m,pilot::SeatAnchor),seat),"the pilot's seat anchor lands on the seat");
    Check(Near(glm::normalize(Direction(m,glm::vec3(0,0,1))),glm::vec3(1,0,0)),"pilot front (+Z) faces the nose (+X)");
    Check(Near(glm::normalize(Direction(m,glm::vec3(0,1,0))),glm::vec3(0,1,0)),"pilot up stays up");
    Check(std::abs(glm::length(Direction(m,glm::vec3(0,1,0)))-.8f)<1e-4f,"uniform scale applied");

    // Light aircraft: both pilots fit the cabin measured in its OBJ.
    const auto& light=Spec("light");
    Check(light.crew.seats.size()==2,"light aircraft has two seated pilots");
    for(const auto& s:light.crew.seats) {
        const glm::mat4 local=pilot::SeatMatrix(s,light.crew.scale);
        glm::vec3 lo(1e9f),hi(-1e9f);
        for(int i=0;i<8;++i) {
            const glm::vec3 corner(i&1?pilot::BoundsMax.x:pilot::BoundsMin.x,
                                   i&2?pilot::BoundsMax.y:pilot::BoundsMin.y,
                                   i&4?pilot::BoundsMax.z:pilot::BoundsMin.z);
            const glm::vec3 p=Point(local,corner);
            lo=glm::min(lo,p);hi=glm::max(hi,p);
        }
        Check(hi.y<2.68f,"head stays under the fuselage top (Y 2.68)");
        Check(lo.y>1.34f,"feet do not go below the cabin floor (Y 1.34..1.42)");
        Check(hi.x<2.96f,"knees and hands stay behind the dashboard (X 2.96)");
        Check(lo.x>1.85f,"back stays in front of the seat back (X 1.88)");
        Check(lo.z>-.69f&&hi.z<.69f,"inside the fuselage width (Z +-0.69)");
    }
    // The two pilots do not overlap: left one ends before the right one starts.
    const auto zRange=[&](const glm::vec3& s,float& lo,float& hi) {
        const glm::mat4 local=pilot::SeatMatrix(s,light.crew.scale);
        lo=1e9f;hi=-1e9f;
        for(float x:{pilot::BoundsMin.x,pilot::BoundsMax.x}) {
            const float z=Point(local,glm::vec3(x,1,0)).z;lo=std::fmin(lo,z);hi=std::fmax(hi,z);
        }
    };
    float leftLo,leftHi,rightLo,rightHi;
    zRange(light.crew.seats[0],leftLo,leftHi);zRange(light.crew.seats[1],rightLo,rightHi);
    Check(leftHi<rightLo,"the two pilots do not overlap");

    // Cockpit camera sits in the left seat: that pilot is hidden, the other drawn,
    // in any aircraft pose (the test uses a rotated and moved aircraft).
    const glm::mat4 aircraft=Matrix_Translate(5237,30,6295)*Matrix_Rotate_Y(.7f)*Matrix_Rotate_X(.2f);
    const glm::vec3 eye=Point(aircraft,light.cockpitEye);
    Check(pilot::CameraInsidePilot(aircraft,light.crew.seats[0],light.crew.scale,eye),"cockpit camera hides the left pilot");
    Check(!pilot::CameraInsidePilot(aircraft,light.crew.seats[1],light.crew.scale,eye),"the right pilot stays visible from the cockpit");
    const glm::vec3 outside=Point(aircraft,glm::vec3(-20,4,0));
    for(const auto& s:light.crew.seats)
        Check(!pilot::CameraInsidePilot(aircraft,s,light.crew.scale,outside),"external camera sees both pilots");

    // The A320 crew comes in a later step.
    Check(Spec("a320").crew.seats.empty(),"A320 has no pilots yet");
    std::printf("pilot_crew: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
