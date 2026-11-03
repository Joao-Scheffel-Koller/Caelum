#include "ui/FrontendInspection.hpp"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {
struct View {const char* name;int w,h;frontend::Page page;std::size_t aircraft;double fraction;};
const View Views[]={
    {"main",1280,720,frontend::Page::Main,0,0},
    {"aircraft",1280,720,frontend::Page::Aircraft,0,0},
    {"main-light",1280,720,frontend::Page::Main,1,0},
    {"controls",1280,720,frontend::Page::Controls,0,0},
    {"pause",1280,720,frontend::Page::Pause,0,0},
    {"loading-start",1280,720,frontend::Page::Loading,0,0},
    {"loading-middle",1280,720,frontend::Page::Loading,0,.5},
    {"loading-end",1280,720,frontend::Page::Loading,0,1},
    {"main-compact",800,600,frontend::Page::Main,0,0},
    {"aircraft-small",640,480,frontend::Page::Aircraft,0,0},
    {"loading-minimum",320,240,frontend::Page::Loading,0,.5}
};
const int FramesPerView=30,Count=sizeof(Views)/sizeof(Views[0]);
const View& Current(int frame) {return Views[std::min(frame/FramesPerView,Count-1)];}
bool ClickControl(frontend::FrontendModel& model,frontend::Control control,int w,int h,std::size_t index=0) {
    for(const auto& button:model.Layout(w,h))if(button.control==control&&
        (control!=frontend::Control::ChooseAircraft||button.aircraftIndex==index)&&button.enabled) {
        model.Click(button.bounds.x+button.bounds.width*.5,button.bounds.y+button.bounds.height*.5,w,h);
        return true;
    }
    return false;
}
}
bool SelectFrontendAircraft(frontend::FrontendModel& model,std::size_t index,int w,int h) {
    model.OpenMain();
    if(!ClickControl(model,frontend::Control::Aircraft,w,h))return false;
    const bool selected=ClickControl(model,frontend::Control::ChooseAircraft,w,h,index);
    ClickControl(model,frontend::Control::Back,w,h);
    return selected&&model.SelectedIndex()==index;
}
void FrontendInspection::Prepare(GLFWwindow* window,frontend::FrontendModel& model) {
    if(frame>=Count*FramesPerView||frame%FramesPerView)return;
    const auto& view=Current(frame);
    glfwSetWindowSize(window,view.w,view.h);
    SelectFrontendAircraft(model,view.aircraft,view.w,view.h);
    switch(view.page) {
    case frontend::Page::Aircraft:ClickControl(model,frontend::Control::Aircraft,view.w,view.h);break;
    case frontend::Page::Controls:ClickControl(model,frontend::Control::Controls,view.w,view.h);break;
    case frontend::Page::Pause:model.OpenPause();break;
    case frontend::Page::Loading:model.BeginLoading();break;
    default:break;
    }
}
void FrontendInspection::Draw(GLFWwindow* window,frontend::FrontendScreen& screen,
                              const frontend::FrontendModel& model,double elapsed) {
    const auto& view=Current(frame);
    if(view.page==frontend::Page::Loading)
        screen.DrawLoading(window,view.fraction,"Preparando o cenario","Santos Dumont -> Galeao",elapsed);
    else screen.Draw(window,model,elapsed);
}
void CaptureFrontendFrame(GLFWwindow* window,const std::string& path) {
    // FONTE: existing glReadPixels protocol in RenderInspection.cpp.
    int w,h;glfwGetFramebufferSize(window,&w,&h);
    if(w<=0||h<=0)throw std::runtime_error("Cannot capture an empty frontend framebuffer");
    GLint alignment;glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glPixelStorei(GL_PACK_ALIGNMENT,1);
    std::vector<unsigned char> pixels(std::size_t(w)*h*3);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());glPixelStorei(GL_PACK_ALIGNMENT,alignment);
    const auto error=glGetError();
    if(error!=GL_NO_ERROR)throw std::runtime_error("Frontend OpenGL error "+std::to_string(error));
    std::ofstream out(path,std::ios::binary);out<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int row=h-1;row>=0;--row)out.write(reinterpret_cast<const char*>(pixels.data()+std::size_t(row)*w*3),w*3);
    if(!out)throw std::runtime_error("Cannot save frontend capture: "+path);
    std::printf("Frontend capture: %s (%dx%d), OpenGL OK\n",path.c_str(),w,h);std::fflush(stdout);
}
bool FrontendInspection::Capture(GLFWwindow* window) {
    if(++frame%FramesPerView==0) {
        const auto& view=Views[frame/FramesPerView-1];
        CaptureFrontendFrame(window,prefix+"-"+view.name+".ppm");
    }
    return frame>=Count*FramesPerView;
}
