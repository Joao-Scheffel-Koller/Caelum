#include "render/RenderInspection.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <cstring>

namespace {
struct View {const char* name;glm::vec3 eye,target,up;};
const View Views[]={
    {"bezier-start",{0,0,0},{0,0,0},{0,1,0}},
    {"runway-low",{5237,7,6220},{5240,0,6470},{0,1,0}},
    {"runway-high",{5237,140,6200},{5240,0,6500},{0,1,0}},
    {"airport-top",{5300,800,7000},{5300,0,7000},{0,0,-1}},
    {"park-north",{4470,450,6500},{4470,0,6500},{0,0,-1}},
    {"park-center",{4470,450,7100},{4470,0,7100},{0,0,-1}},
    {"park-south",{4300,650,7700},{4300,0,7700},{0,0,-1}},
    {"city",{3700,500,5900},{4200,0,5700},{0,1,0}},
    {"galeao",{1400,600,-4200},{1800,0,-4000},{0,1,0}},
    {"aircraft",{5190,14,6295},{5237,6,6295},{0,1,0}},
    {"runway-move-a",{5237,12,6250},{5240,0,6500},{0,1,0}},
    {"runway-move-b",{5237,12,6260},{5240,0,6510},{0,1,0}},
    {"runway-move-c",{5237,12,6270},{5240,0,6520},{0,1,0}},
    {"cockpit",{0,0,0},{0,0,0},{0,1,0}},
    {"bezier-later",{0,0,0},{0,0,0},{0,1,0}}
};
const int Count=sizeof(Views)/sizeof(Views[0]);
const int FramesPerView=30;
}

bool RenderInspection::Cockpit() const {
    return std::strcmp(Views[frame/FramesPerView].name,"cockpit")==0;
}

CameraPose RenderInspection::Camera(const CameraFrame& aircraft,const FlightPose& traffic) const {
    const auto& view=Views[frame/FramesPerView];
    CameraPose pose;
    if(std::strncmp(view.name,"bezier-",7)==0) {
        pose.position=traffic.position-traffic.forward*90.0f+traffic.right*65.0f+glm::vec3(0,25,0);
        pose.forward=glm::normalize(traffic.position-pose.position);
        if(frame%FramesPerView==0) {
            std::printf("Traffic %s: (%.3f, %.3f, %.3f)\n",view.name,traffic.position.x,traffic.position.y,traffic.position.z);
            std::fflush(stdout);
        }
        return pose;
    }
    if(Cockpit()) {
        pose.position=glm::vec3(aircraft.aircraftModel*glm::vec4(aircraft.cockpitEyeLocal,1));
        pose.forward=glm::normalize(glm::vec3(aircraft.aircraftModel[0]));
        pose.up=glm::normalize(glm::vec3(aircraft.aircraftModel[1]));
        pose.nearPlane=.05f;
        return pose;
    }
    pose.position=view.eye;
    pose.forward=glm::normalize(view.target-view.eye);
    pose.up=view.up;
    return pose;
}

bool RenderInspection::Capture(GLFWwindow* window) {
    if(++frame%FramesPerView!=0)return false;
    int width,height;glfwGetFramebufferSize(window,&width,&height);
    std::vector<unsigned char> pixels(size_t(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    const std::string file=prefix+"-"+Views[frame/FramesPerView-1].name+".ppm";
    std::ofstream output(file,std::ios::binary);
    output<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int row=height-1;row>=0;--row)
        output.write(reinterpret_cast<const char*>(pixels.data()+size_t(row)*width*3),width*3);
    if(!output)throw std::runtime_error("Cannot save inspection: "+file);
    const GLenum error=glGetError();
    if(error!=GL_NO_ERROR)throw std::runtime_error("OpenGL inspection error "+std::to_string(error));
    std::printf("Inspection: %s (%dx%d), OpenGL OK\n",file.c_str(),width,height);
    std::fflush(stdout);
    return frame>=Count*FramesPerView;
}
