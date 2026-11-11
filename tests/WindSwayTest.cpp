// AI-assisted CPU regressions for the tree wind (same formula as the shader).
#include "render/WindSway.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <glm/geometric.hpp>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
float Along(const WindParameters& w,const glm::vec3& offset) {
    return offset.x*w.direction.x+offset.z*w.direction.y;
}
} // namespace

int main() {
    const WindParameters wind;
    const glm::vec3 tree(4277.1f,0.0f,6639.6f);
    Check(std::abs(glm::length(wind.direction)-1.0f)<1e-4f,"wind direction is a unit vector");

    // The base stays rooted at every instant; the top moves.
    float maxTop=0,maxMiddle=0;
    for(int i=0;i<2000;++i) {
        const float t=i*0.01f;
        Check(glm::length(WindOffset(wind,tree,0.0f,t))<1e-6f,"base never moves");
        maxTop=std::max(maxTop,std::abs(Along(wind,WindOffset(wind,tree,1.0f,t))));
        maxMiddle=std::max(maxMiddle,std::abs(Along(wind,WindOffset(wind,tree,0.5f,t))));
        Check(std::abs(WindOffset(wind,tree,1.0f,t).y)<1e-6f,"sway is horizontal");
    }
    Check(maxTop>0.5f*wind.amplitude,"the top sways visibly");
    Check(maxTop<=wind.amplitude+wind.flutter+1e-4f,"sway never exceeds amplitude + rustle");
    Check(maxMiddle<0.5f*maxTop,"trees bend: the middle moves much less than the top (v^2)");

    // Time-based: the offset changes with time and is smooth (no jumps).
    const glm::vec3 a=WindOffset(wind,tree,1.0f,1.0f),b=WindOffset(wind,tree,1.0f,2.5f);
    Check(glm::length(a-b)>0.01f,"motion depends on time");
    for(int i=0;i<1000;++i) {
        const float t=i*(1.0f/60.0f);
        const float step=glm::length(WindOffset(wind,tree,1.0f,t+1.0f/60.0f)-WindOffset(wind,tree,1.0f,t));
        Check(step<0.05f,"frame-to-frame motion is smooth at 60 FPS");
    }

    // Neighbouring trees move almost together; distant ones are out of step.
    float nearDiff=0,farDiff=0;
    const glm::vec3 neighbour=tree+glm::vec3(3,0,0),distant=tree+glm::vec3(wind.direction.x,0,wind.direction.y)*60.0f;
    for(int i=0;i<600;++i) {
        const float t=i*0.05f;
        const float base=Along(wind,WindOffset(wind,tree,1.0f,t));
        nearDiff=std::max(nearDiff,std::abs(base-Along(wind,WindOffset(wind,neighbour,1.0f,t))));
        farDiff=std::max(farDiff,std::abs(base-Along(wind,WindOffset(wind,distant,1.0f,t))));
    }
    Check(nearDiff<0.1f,"neighbouring trees sway almost together");
    Check(farDiff>0.25f,"trees 60 m apart along the wind are out of step (travelling gust)");

    // The gust travels downwind: a tree further along sees the same sway later.
    const float delay=wind.wavenumber*20.0f/wind.angularFrequency;
    const glm::vec3 downwind=tree+glm::vec3(wind.direction.x,0,wind.direction.y)*20.0f;
    const float upNow=Along(wind,WindOffset(wind,tree,1.0f,3.0f)-WindOffset(wind,tree,0.0f,3.0f));
    const float downLater=Along(wind,WindOffset(wind,downwind,1.0f,3.0f+delay));
    Check(std::abs(upNow-downLater)<0.05f,"main wave travels with the wind");

    // Clock: advances with dt only.
    WindClock clock;
    clock.Update(0.5);clock.Update(-1.0);clock.Update(0.25);
    Check(std::abs(clock.Time()-0.75f)<1e-6f,"clock accumulates elapsed time and ignores negative dt");

    std::printf("wind_sway: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
