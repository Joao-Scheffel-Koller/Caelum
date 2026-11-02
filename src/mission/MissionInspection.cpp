#include "mission/MissionInspection.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {
struct View {const char* name;int width,height;mission::Stage stage;const char* instruction;};
const View Views[]={
    {"departure",1280,720,mission::Stage::Departure,nullptr},
    {"en-route",1280,720,mission::Stage::EnRoute,nullptr},
    {"approach",1280,720,mission::Stage::Approach,nullptr},
    {"landing",800,600,mission::Stage::Landing,nullptr},
    {"completed",1280,720,mission::Stage::Completed,nullptr},
    {"failed",800,600,mission::Stage::Failed,nullptr},
    {"approach-compact",640,480,mission::Stage::Approach,nullptr},
    {"landing-small",320,360,mission::Stage::Landing,nullptr},
    {"failed-minimum",320,240,mission::Stage::Failed,nullptr},
    {"landing-align-minimum",320,240,mission::Stage::Landing,"Alinhe o aviao com a pista."},
    {"landing-slow-compact",640,480,mission::Stage::Landing,"Reduza para ate 18 km/h na pista."}
};
const int Count=sizeof(Views)/sizeof(Views[0]);
const int FramesPerView=30;
const View& Current(int frame) {return Views[std::min(frame/FramesPerView,Count-1)];}
const char* StageName(mission::Stage stage) {
    switch(stage) {
    case mission::Stage::Departure:return "Departure";
    case mission::Stage::EnRoute:return "EnRoute";
    case mission::Stage::Approach:return "Approach";
    case mission::Stage::Landing:return "Landing";
    case mission::Stage::Completed:return "Completed";
    case mission::Stage::Failed:return "Failed";
    }
    return "Unknown";
}
mission::MissionSnapshot Fixture(mission::Stage stage) {
    mission::MissionSnapshot snapshot;
    snapshot.stage=stage;
    const auto& origin=mission::SantosDumont();
    const auto& destination=mission::Galeao();
    const auto& runway=destination.runways.front();
    const glm::dvec3 departure(5237.1933,0,6295.7127);
    glm::dvec3 position=departure;
    snapshot.target=(runway.start+runway.end)*.5;
    snapshot.targetName=destination.name+" - pista "+runway.name;
    snapshot.departed=stage!=mission::Stage::Departure;
    snapshot.flightSeconds=180;
    const auto axis=glm::normalize(runway.end-runway.start);
    switch(stage) {
    case mission::Stage::Departure:
        snapshot.target=origin.runways.front().end;
        snapshot.targetName=origin.name+" - pista "+origin.runways.front().name;
        snapshot.instruction="Decole do "+origin.name+" para iniciar a missao.";
        snapshot.originArmed=true;snapshot.flightSeconds=0;
        break;
    case mission::Stage::EnRoute:
        snapshot.target=runway.start-axis*1000.0;
        snapshot.instruction="Voe ate o "+destination.name+" e prepare a aproximacao.";
        snapshot.flightSeconds=45;
        break;
    case mission::Stage::Approach:
        position=runway.start-axis*600.0;position.y=120;
        snapshot.instruction="Alinhe e pouse com seguranca numa pista do "+destination.name+".";
        break;
    case mission::Stage::Landing:
        position=runway.start+axis*500.0;
        snapshot.instruction="Mantenha o aviao na pista por 2 s.";
        snapshot.landingSeconds=1.25;
        break;
    case mission::Stage::Completed:
        position=snapshot.target;
        snapshot.instruction="Missao concluida! Pouso no "+destination.name+" confirmado.";
        snapshot.landingSeconds=2;
        break;
    case mission::Stage::Failed:
        position=(departure+snapshot.target)*.5;position.y=0;
        snapshot.instruction="Missao falhou: acidente. Enter: reiniciar.";
        snapshot.flightSeconds=93;
        break;
    }
    snapshot.distanceMeters=std::hypot(snapshot.target.x-position.x,snapshot.target.z-position.z);
    snapshot.bearingDegrees=mission::BearingToTarget(position,snapshot.target);
    return snapshot;
}
}

void MissionInspection::Prepare(GLFWwindow* window) const {
    if(frame>=Count*FramesPerView||frame%FramesPerView!=0)return;
    const auto& view=Current(frame);
    glfwSetWindowSize(window,view.width,view.height);
    viewStarted=glfwGetTime();
}
mission::MissionSnapshot MissionInspection::Preview(const mission::MissionSnapshot& live) const {
    displayed=frame<Count*FramesPerView?Fixture(Current(frame).stage):live;
    if(frame<Count*FramesPerView&&Current(frame).instruction) {
        displayed.instruction=Current(frame).instruction;displayed.landingSeconds=0;
    }
    return displayed;
}
bool MissionInspection::Capture(GLFWwindow* window) {
    if(frame>=Count*FramesPerView)return true;
    if(++frame%FramesPerView!=0)return false;
    const auto& view=Views[frame/FramesPerView-1];
    int width,height;glfwGetFramebufferSize(window,&width,&height);
    if(width<=0||height<=0)throw std::runtime_error("Mission UI inspection has an empty framebuffer");
    std::vector<unsigned char> pixels(size_t(width)*height*3);
    // FONTE: existing readback protocol in ui/HudInspection.cpp and the
    // professor's glReadPixels-based smoke test in main.cpp.
    GLint alignment;glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT,alignment);
    const auto error=glGetError();
    if(error!=GL_NO_ERROR)throw std::runtime_error("Mission UI inspection OpenGL error "+std::to_string(error));
    const auto file=prefix+"-"+view.name+".ppm";
    std::ofstream output(file,std::ios::binary);
    output<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int row=height-1;row>=0;--row)
        output.write(reinterpret_cast<const char*>(pixels.data()+size_t(row)*width*3),width*3);
    if(!output)throw std::runtime_error("Cannot save mission UI inspection: "+file);
    const double elapsed=std::max(0.0,glfwGetTime()-viewStarted);
    std::printf("Mission inspection fixture UI (not gameplay): %s (%dx%d), OpenGL OK; "
                "stage=%s distance=%.2f m bearing=%.2f flight=%.2f s landing=%.2f s "
                "mean_dt=%.5f s frames=%d total_frame=%d/%d\n",file.c_str(),width,height,
                StageName(displayed.stage),displayed.distanceMeters,displayed.bearingDegrees,
                displayed.flightSeconds,displayed.landingSeconds,elapsed/FramesPerView,
                FramesPerView,frame,Count*FramesPerView);
    std::fflush(stdout);
    return frame>=Count*FramesPerView;
}
