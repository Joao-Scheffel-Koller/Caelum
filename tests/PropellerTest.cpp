// AI-assisted regressions for the animated propeller (docs/propeller.md).
// Usage: propeller_test <path to data>
#include "aircraft/Propeller.hpp"
#include "aircraft/AircraftCatalog.hpp"
#include "matrices.h"
#include <tiny_obj_loader.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
const float Pi=3.14159265f;
// Runs a propeller for `seconds` at a fixed frame rate.
propeller::Spin Run(float throttle,float seconds,float fps,float maxRpm=2700) {
    propeller::Spin spin;
    const int frames=int(std::lround(seconds*fps));
    for(int i=0;i<frames;++i)spin.Update(1.0/fps,throttle,maxRpm);
    return spin;
}
float AngleDifference(float a,float b) {
    float d=std::fmod(std::abs(a-b),2*Pi);
    return std::min(d,2*Pi-d);
}
std::vector<glm::vec3> Place(const std::vector<glm::vec3>& points,const glm::mat4& m) {
    std::vector<glm::vec3> result;
    for(const auto& p:points)result.push_back(glm::vec3(m*glm::vec4(p,1)));
    return result;
}
// Largest distance from a point of a to the nearest point of b.
float Mismatch(const std::vector<glm::vec3>& a,const std::vector<glm::vec3>& b) {
    float worst=0;
    for(const auto& p:a) {
        float nearest=1e9f;
        for(const auto& q:b)nearest=std::min(nearest,glm::length(p-q));
        worst=std::max(worst,nearest);
    }
    return worst;
}
const AircraftSpec& Light() {
    for(const auto& spec:AircraftCatalog())if(spec.id=="light")return spec;
    std::fprintf(stderr,"FAIL: light aircraft missing\n");std::exit(EXIT_FAILURE);
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2){std::fprintf(stderr,"usage: propeller_test <data>\n");return EXIT_FAILURE;}

    // --- Engine speed follows the throttle, gradually, independent of the FPS.
    Check(Run(0,5,60).Rpm()==0&&Run(0,5,60).Angle()==0,"throttle 0: the propeller stays still");
    Check(std::abs(Run(1,8,60).Rpm()-2700)<5,"full throttle reaches the maximum rpm");
    Check(std::abs(Run(.5f,8,60).Rpm()-1350)<3,"rpm is proportional to the throttle");
    const float early=Run(1,.3f,60).Rpm();
    Check(early>300&&early<1500,"the engine spools up gradually, not instantly");
    Check(std::abs(Run(.6f,1.5f,30).Rpm()-Run(.6f,1.5f,144).Rpm())<1,"same rpm at 30 and 144 FPS (time-based)");
    // Steady low speed: the angle after 2 s is the same at any frame rate.
    {
        propeller::Spin a,b;
        for(int i=0;i<600;++i)a.Update(1.0/60,.05f,2700);
        for(int i=0;i<300;++i)b.Update(1.0/30,.05f,2700);
        Check(AngleDifference(a.Angle(),b.Angle())<.02f,"same angle at 60 and 30 FPS after the same time");
        const float before=a.Angle();
        a.Update(0.0,1,2700);a.Update(-1.0,1,2700);
        Check(a.Angle()==before,"paused (dt=0) or a bad clock: nothing moves");
    }

    // --- Anti-aliasing: the shown rotation never exceeds half the blade
    // symmetry (60 deg) per frame at 30 FPS or more, at any engine speed.
    for(float rpm=0;rpm<=3000;rpm+=50) {
        const float degreesPerFrame=propeller::DisplayedRpm(rpm)*360.0f/60.0f/propeller::MinimumFps;
        Check(degreesPerFrame<=propeller::SymmetryDegrees*.5f+1e-3f,"no wagon-wheel effect at 30 FPS");
    }
    Check(std::abs(propeller::VisibleRpm-300)<1e-3f,"solid blades shown up to 300 rpm");
    Check(propeller::DisplayedRpm(135)==135,"slow propeller turns at its true speed");

    // --- Blades fade into the translucent disc as the speed rises.
    Check(propeller::BladeOpacity(0)==1&&propeller::DiscOpacity(0)==0,"stopped: solid blades, no disc");
    Check(propeller::BladeOpacity(300)==1&&propeller::DiscOpacity(300)==0,"up to 300 rpm: solid blades, no disc");
    Check(propeller::BladeOpacity(2700)<.2f&&propeller::DiscOpacity(2700)>=.25f&&propeller::DiscOpacity(2700)<.5f,
          "full power: translucent disc, faint blades");
    for(float rpm=0;rpm<3000;rpm+=50) {
        Check(propeller::BladeOpacity(rpm+50)<=propeller::BladeOpacity(rpm),"blades only fade as rpm rises");
        Check(propeller::DiscOpacity(rpm+50)>=propeller::DiscOpacity(rpm),"disc only forms as rpm rises");
    }

    Check(propeller::BladeOpacity(0,true)==propeller::CockpitBladeOpacity,"cockpit: stopped blades do not hide the view");
    Check(propeller::BladeOpacity(2700,true)==propeller::BladeOpacity(2700),"cockpit: blur as faint as outside");
    Check(propeller::DiscOpacity(2700,true)<=.1f&&propeller::DiscOpacity(2700,true)>0,"cockpit: the disc barely tints the runway");

    // --- Asset: the light aircraft's propeller as described in the catalog.
    const auto& spec=Light().propeller;
    Check(spec.Present()&&spec.maxRpm==2700,"light aircraft has a propeller");
    const std::string folder=std::string(argv[1])+"/light_aircraft/";
    tinyobj::attrib_t attrib;std::vector<tinyobj::shape_t> shapes;std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    Check(tinyobj::LoadObj(&attrib,&shapes,&materials,&warning,&error,(folder+"aircraft.obj").c_str(),folder.c_str(),true),
          "light aircraft OBJ loads");
    glm::vec3 allMin(1e9f),allMax(-1e9f),bodyMin(1e9f),bodyMax(-1e9f),partMin(1e9f),partMax(-1e9f);
    std::vector<glm::vec3> blades[3],spinner;   // Vertices of each piece.
    size_t found=0;
    for(const auto& shape:shapes) {
        const bool part=std::find(spec.blades.begin(),spec.blades.end(),shape.name)!=spec.blades.end()||
                        std::find(spec.spinner.begin(),spec.spinner.end(),shape.name)!=spec.spinner.end();
        found+=part;
        for(const auto& index:shape.mesh.indices) {
            const glm::vec3 v(attrib.vertices[3*index.vertex_index],attrib.vertices[3*index.vertex_index+1],
                              attrib.vertices[3*index.vertex_index+2]);
            allMin=glm::min(allMin,v);allMax=glm::max(allMax,v);
            (part?partMin:bodyMin)=glm::min(part?partMin:bodyMin,v);
            (part?partMax:bodyMax)=glm::max(part?partMax:bodyMax,v);
            for(int b=0;b<3;++b)if(shape.name==spec.blades[b])blades[b].push_back(v);
            if(part&&shape.name==spec.spinner[0])spinner.push_back(v);
        }
    }
    Check(found==spec.blades.size()+spec.spinner.size(),"every propeller object exists in the OBJ");
    Check(spec.spinner.size()==1&&spec.spinner[0]=="wingRoot","the spinner is split apart from the blades (stays solid)");

    // --- Rotation axis. A propeller turned by 120 degrees must coincide with
    // itself: that fails for any wrong axis, tilt or stretch. (The previous
    // check compared vertices at equal X, which are only mirrored pairs, so it
    // passed for almost any axis.)
    std::vector<glm::vec3> allBlades;
    for(const auto& b:blades)allBlades.insert(allBlades.end(),b.begin(),b.end());
    Check(!blades[0].empty()&&!blades[1].empty()&&!blades[2].empty()&&!spinner.empty(),"blades and spinner read from the OBJ");
    const float third=2*Pi/3;
    for(float angle:{0.0f,.4f,1.3f,2.9f,4.7f})
        Check(Mismatch(Place(allBlades,propeller::BladeMatrix(spec,angle+third)),
                       Place(allBlades,propeller::BladeMatrix(spec,angle)))<.001f,"blades coincide after a 120 degree turn");
    Check(Mismatch(Place(spinner,propeller::SpinnerMatrix(spec,third)),spinner)<.006f,"spinner coincides after a 120 degree turn");
    Check(Mismatch(Place(spinner,propeller::SpinnerMatrix(spec,0)),spinner)<1e-4f,"at rest the spinner is where the OBJ has it (fits the cowling)");
    // The check must be able to fail: the first version (about X) does.
    const glm::mat4 wrong=Matrix_Translate(4.30f,1.9327f,0)*Matrix_Rotate_X(third)*Matrix_Translate(-4.30f,-1.9327f,0);
    Check(Mismatch(Place(allBlades,wrong),allBlades)>.2f,"turning about X is detected as wrong");

    // No wobble: at every angle the blades stay in the disc's plane and every
    // tip keeps the disc's radius (they neither tilt nor stretch).
    const glm::mat4 toDisc=glm::inverse(propeller::DiscMatrix(spec));
    for(int step=0;step<72;++step) {
        const glm::mat4 m=toDisc*propeller::BladeMatrix(spec,2*Pi*step/72);
        for(const auto& b:blades) {
            float tip=0;
            for(const auto& v:Place(b,m)) {
                Check(std::abs(v.x)<.04f,"blades stay in the disc's plane (blade thickness only)");
                tip=std::max(tip,glm::length(glm::vec2(v.y,v.z)));
            }
            Check(std::abs(tip-spec.tipRadius)<.002f,"every blade keeps the disc's radius at every angle");
        }
    }
    float lowest=1e9f;
    for(int step=0;step<72;++step)for(const auto& v:Place(allBlades,propeller::BladeMatrix(spec,2*Pi*step/72)))lowest=std::min(lowest,v.y);
    Check(lowest>.5f,"blades clear the ground");
    // Direction: the top blade's tip moves to the right (+Z).
    const glm::vec3 top=*std::max_element(blades[0].begin(),blades[0].end(),[](const glm::vec3& a,const glm::vec3& b){return a.y<b.y;});
    Check(Place({top},propeller::BladeMatrix(spec,.3f))[0].z>.2f,"clockwise seen from the cockpit (up -> right)");
    std::printf("plane tilt %.2f deg, lowest blade point Y %.3f m\n",propeller::PlaneTilt(spec)*180/Pi,lowest);

    // Pivot: without the propeller the bounds (and so the aircraft's position)
    // would change; combining both pieces gives back the original bounds.
    Check(bodyMax.x<allMax.x-.2f,"the propeller is the front of the aircraft: removing it shortens the bounds");
    const glm::vec3 combinedMin=glm::min(bodyMin,partMin),combinedMax=glm::max(bodyMax,partMax);
    Check(glm::length(combinedMin-allMin)<1e-6f&&glm::length(combinedMax-allMax)<1e-6f,
          "body + propeller bounds = whole model bounds (pivot unchanged)");
    std::printf("pivot X without the propeller would move %.3f m; combined pivot unchanged\n",
                ((bodyMin.x+bodyMax.x)-(allMin.x+allMax.x))*.5f);
    std::printf("propeller: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
