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

    // --- Matrix: rotation about the axis through the hub, parallel to X.
    const glm::vec3 hub(4.30f,1.9327f,0.0f);
    const glm::mat4 quarter=propeller::BladeMatrix(hub,Pi*.5f);
    const glm::vec3 onAxis=glm::vec3(quarter*glm::vec4(3.0f,hub.y,hub.z,1));
    Check(glm::length(onAxis-glm::vec3(3.0f,hub.y,hub.z))<1e-5f,"points on the axis do not move");
    const glm::vec3 tip=glm::vec3(quarter*glm::vec4(hub+glm::vec3(0,1,0),1));
    Check(glm::length(tip-(hub+glm::vec3(0,0,1)))<1e-5f,"positive angle: up -> right (+Z), clockwise from the cockpit");
    const glm::vec3 anywhere(4.2f,2.5f,-.7f);
    for(float a=0;a<6.3f;a+=.7f) {
        const glm::vec3 p=glm::vec3(propeller::BladeMatrix(hub,a)*glm::vec4(anywhere,1));
        Check(std::abs(p.x-anywhere.x)<1e-5f,"blades stay in their plane (X unchanged)");
        Check(std::abs(glm::length(glm::vec2(p.y-hub.y,p.z-hub.z))-glm::length(glm::vec2(anywhere.y-hub.y,anywhere.z-hub.z)))<1e-5f,
              "distance to the axis is kept (rigid rotation)");
    }

    // --- Asset: the light aircraft's propeller as described in the catalog.
    const auto& spec=Light().propeller;
    Check(spec.Present()&&spec.maxRpm==2700,"light aircraft has a propeller");
    const std::string folder=std::string(argv[1])+"/light_aircraft/";
    tinyobj::attrib_t attrib;std::vector<tinyobj::shape_t> shapes;std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    Check(tinyobj::LoadObj(&attrib,&shapes,&materials,&warning,&error,(folder+"aircraft.obj").c_str(),folder.c_str(),true),
          "light aircraft OBJ loads");
    glm::vec3 allMin(1e9f),allMax(-1e9f),bodyMin(1e9f),bodyMax(-1e9f),partMin(1e9f),partMax(-1e9f);
    float tipRadius=0;
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
            if(part)tipRadius=std::max(tipRadius,glm::length(glm::vec2(v.y-spec.hub.y,v.z-spec.hub.z)));
        }
        if(shape.name=="wingRoot") {
            // The spinner is round about the axis: vertices at the same X
            // (1 mm slices) all have the same distance to it.
            std::vector<std::pair<long,float>> rings;
            for(const auto& index:shape.mesh.indices) {
                const float* v=&attrib.vertices[3*index.vertex_index];
                rings.push_back({std::lround(v[0]*1000),glm::length(glm::vec2(v[1]-spec.hub.y,v[2]-spec.hub.z))});
            }
            std::sort(rings.begin(),rings.end());
            float spread=0;
            for(size_t i=0,j=0;i<rings.size();i=j) {
                float lo=1e9f,hi=-1e9f;
                for(j=i;j<rings.size()&&rings[j].first==rings[i].first;++j){lo=std::min(lo,rings[j].second);hi=std::max(hi,rings[j].second);}
                spread=std::max(spread,hi-lo);
            }
            Check(spread<.005f,"the spinner is centred on the rotation axis (it does not wobble)");
        }
    }
    Check(found==spec.blades.size()+spec.spinner.size(),"every propeller object exists in the OBJ");
    Check(spec.spinner.size()==1&&spec.spinner[0]=="wingRoot","the spinner is split apart from the blades (stays solid)");
    Check(std::abs(tipRadius-spec.tipRadius)<.01f,"blur disc radius = longest blade");
    Check(spec.hub.x>=partMin.x&&spec.hub.x<=partMax.x,"disc sits in the blades' plane");
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
