#include "ui/HudInspection.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {
struct View {
    const char* name;
    int width,height;
    CameraType camera;
    double speedKmh,altitudeMeters,throttlePercent,headingDegrees,pitchDegrees,rollDegrees;
    bool real,invalid;
};
const View Views[]={
    {"orbit-real",1280,720,CameraType::LOOK_AT,0,0,0,0,0,0,true,false},
    {"cockpit-north",1280,720,CameraType::COCKPIT,216,800,60,0,0,0,false,false},
    {"free-east-bank",1280,720,CameraType::FREE,500,1500,85,90,20,30,false,false},
    {"orbit-west-descent",800,600,CameraType::LOOK_AT,216,800,60,270,-15,-35,false,false},
    {"cockpit-inverted",640,480,CameraType::COCKPIT,216,800,60,0,0,180,false,false},
    {"free-invalid",320,360,CameraType::FREE,0,0,0,0,0,0,false,true},
    {"orbit-high-altitude",1920,1080,CameraType::LOOK_AT,900,12345,60,0,0,0,false,false},
    {"orbit-max-speed",1280,720,CameraType::LOOK_AT,1200,800,100,0,0,0,false,false},
    {"orbit-altitude-before-wrap",1280,720,CameraType::LOOK_AT,216,999,60,0,0,0,false,false},
    {"orbit-altitude-after-wrap",1280,720,CameraType::LOOK_AT,216,1001,60,0,0,0,false,false},
    {"orbit-small-window",320,240,CameraType::LOOK_AT,216,800,60,90,20,30,false,false}
};
const int Count=sizeof(Views)/sizeof(Views[0]);
// Let the visual needle response settle after the intentionally abrupt fixture
// changes (for example 12345 m -> 800 m), before comparing the captured scales.
const int FramesPerView=90;
const View& Current(int frame) {return Views[std::min(frame/FramesPerView,Count-1)];}
}

void HudInspection::Prepare(GLFWwindow* window,CameraController& camera,const CameraFrame& aircraft) const {
    if(frame>=Count*FramesPerView||frame%FramesPerView!=0)return;
    const auto& view=Current(frame);
    glfwSetWindowSize(window,view.width,view.height);
    camera.SetMode(view.camera,aircraft);
    camera.Reset(aircraft);
}

FlightTelemetry HudInspection::Telemetry(const FlightTelemetry& real) const {
    const auto& view=Current(frame);
    if(view.real)displayed=real;
    else if(view.invalid)displayed=FlightTelemetry{};
    else {
        // Build synthetic vectors independently of the HUD renderer. The world
        // uses north=-Z and east=+X; positive roll lowers the right wing.
        const double radians=std::acos(-1.0)/180;
        const double heading=view.headingDegrees*radians;
        const double pitch=view.pitchDegrees*radians;
        const double roll=view.rollDegrees*radians;
        const glm::dvec3 forward(std::sin(heading)*std::cos(pitch),std::sin(pitch),
                                -std::cos(heading)*std::cos(pitch));
        const glm::dvec3 right(std::cos(heading),0,std::sin(heading));
        const glm::dvec3 up=glm::cross(right,forward)*std::cos(roll)+right*std::sin(roll);
        displayed=CalculateFlightTelemetry(glm::vec3(0,float(view.altitudeMeters),0),
            glm::vec3(forward*(view.speedKmh/3.6)),glm::vec3(forward),glm::vec3(up),
            view.throttlePercent/100);
    }
    return displayed;
}

bool HudInspection::Capture(GLFWwindow* window) {
    if(frame>=Count*FramesPerView)return true;
    if(++frame%FramesPerView!=0)return false;
    const auto& view=Views[frame/FramesPerView-1];
    int width,height;
    glfwGetFramebufferSize(window,&width,&height);
    if(width<=0||height<=0)throw std::runtime_error("HUD inspection has an empty framebuffer");
    std::vector<unsigned char> pixels(size_t(width)*height*3);
    // FONTE: existing screenshot protocol in src/render/RenderInspection.cpp.
    GLint packAlignment;
    glGetIntegerv(GL_PACK_ALIGNMENT,&packAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT,packAlignment);
    const GLenum error=glGetError();
    if(error!=GL_NO_ERROR)
        throw std::runtime_error("HUD inspection OpenGL error "+std::to_string(error));
    const std::string file=prefix+"-"+view.name+".ppm";
    std::ofstream output(file,std::ios::binary);
    output<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int row=height-1;row>=0;--row)
        output.write(reinterpret_cast<const char*>(pixels.data()+size_t(row)*width*3),width*3);
    if(!output)throw std::runtime_error("Cannot save HUD inspection: "+file);
    std::printf("HUD inspection: %s (%dx%d), OpenGL OK; speed=%.2f km/h altitude=%.2f m "
                "throttle=%.2f%% heading=%.2f pitch=%.2f roll=%.2f; valid=%d%d%d%d%d\n",
        file.c_str(),width,height,displayed.speedKmh,displayed.altitudeMeters,
        displayed.throttlePercent,displayed.headingDegrees,displayed.pitchDegrees,
        displayed.rollDegrees,int(displayed.speedValid),int(displayed.altitudeValid),
        int(displayed.throttleValid),int(displayed.headingValid),int(displayed.attitudeValid));
    std::fflush(stdout);
    return frame>=Count*FramesPerView;
}
