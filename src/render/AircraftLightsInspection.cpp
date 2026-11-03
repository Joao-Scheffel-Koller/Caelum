#include "render/AircraftLightsInspection.hpp"
#include "render/AircraftLightRenderer.hpp"
#include "render/ModelRenderer.hpp"
#include "aircraft/AircraftCatalog.hpp"
#include "camera/CameraController.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

// FONTE: professor's include/matrices.h, already defined by main.cpp.
glm::mat4 Matrix_Camera_View(glm::vec4,glm::vec4,glm::vec4);
glm::mat4 Matrix_Perspective(float,float,float,float);
glm::mat4 Matrix_Translate(float,float,float);
glm::mat4 Matrix_Rotate_Y(float);
glm::mat4 Matrix_Scale(float,float,float);

namespace {
void Check(bool condition,const std::string& name) {
    std::printf("Aircraft lights inspection: %s %s\n",condition?"PASS":"FAIL",name.c_str());
    std::fflush(stdout);
    if(!condition)throw std::runtime_error("Aircraft lights inspection: "+name);
}
void Pump(GLFWwindow* window) {
    glfwPollEvents();
    if(glfwWindowShouldClose(window))throw std::runtime_error("Aircraft lights inspection cancelled");
}
struct Pixels {
    int width=0,height=0;
    std::vector<unsigned char> rgb;
    unsigned long long Sum() const {
        unsigned long long result=0;for(auto value:rgb)result+=value;return result;
    }
};
Pixels Read(GLFWwindow* window) {
    Pixels p;glfwGetFramebufferSize(window,&p.width,&p.height);
    if(p.width<=0||p.height<=0)throw std::runtime_error("Empty aircraft lights inspection framebuffer");
    p.rgb.resize(std::size_t(p.width)*p.height*3);
    GLint alignment;glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
    glReadPixels(0,0,p.width,p.height,GL_RGB,GL_UNSIGNED_BYTE,p.rgb.data());
    glPixelStorei(GL_PACK_ALIGNMENT,alignment);
    const GLenum error=glGetError();
    if(error!=GL_NO_ERROR)throw std::runtime_error("Aircraft lights OpenGL error "+std::to_string(error));
    return p;
}
void Save(const Pixels& p,const std::string& file) {
    std::ofstream output(file,std::ios::binary);
    output<<"P6\n"<<p.width<<" "<<p.height<<"\n255\n";
    for(int y=p.height-1;y>=0;--y)
        output.write(reinterpret_cast<const char*>(p.rgb.data()+std::size_t(y)*p.width*3),p.width*3);
    if(!output)throw std::runtime_error("Cannot save aircraft lights inspection: "+file);
    std::printf("Aircraft lights capture: %s (%dx%d)\n",file.c_str(),p.width,p.height);
}
glm::mat4 View(const CameraPose& camera) {
    return Matrix_Camera_View(glm::vec4(camera.position,1),glm::vec4(camera.forward,0),glm::vec4(camera.up,0));
}
std::array<GLint,13> State() {
    std::array<GLint,13> state;
    const GLenum properties[]={GL_CURRENT_PROGRAM,GL_DEPTH_WRITEMASK,GL_DEPTH_FUNC,
        GL_BLEND_SRC_RGB,GL_BLEND_DST_RGB,GL_BLEND_SRC_ALPHA,GL_BLEND_DST_ALPHA,
        GL_BLEND_EQUATION_RGB,GL_BLEND_EQUATION_ALPHA,GL_VERTEX_ARRAY_BINDING};
    for(std::size_t i=0;i<10;++i)glGetIntegerv(properties[i],&state[i]);
    state[10]=glIsEnabled(GL_DEPTH_TEST);state[11]=glIsEnabled(GL_BLEND);state[12]=glIsEnabled(GL_CULL_FACE);
    return state;
}
double PatternTime(aircraftlights::Pattern pattern,bool on,double offset=0) {
    double best=0;float value=aircraftlights::Intensity(pattern,0,offset);
    for(int i=1;i<=3000;++i) {
        const double time=i*.001;const float candidate=aircraftlights::Intensity(pattern,time,offset);
        if(on?candidate>value:candidate<value){value=candidate;best=time;}
    }
    return best;
}
void GpuChecks(GLFWwindow* window,GLuint sceneProgram,AircraftLightRenderer& renderer,const glm::mat4& projection) {
    aircraftlights::Light lamp;
    lamp.position=glm::vec3(.7f,.2f,-.35f);lamp.color=glm::vec3(1,.15f,.05f);
    lamp.radius=.25f;lamp.haloRadius=.85f;lamp.pattern=aircraftlights::Pattern::Beacon;lamp.phaseOffset=0;
    const std::vector<aircraftlights::Light> lights(1,lamp);
    const double on=PatternTime(lamp.pattern,true),off=PatternTime(lamp.pattern,false);
    const glm::mat4 linear=Matrix_Rotate_Y(.8f)*Matrix_Scale(1.3f,1.3f,1.3f);
    const auto render=[&](LightRenderPass pass,double time,double depth,const glm::vec3& offset) {
        Pump(window);glUseProgram(sceneProgram);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ZERO);
        glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_REVERSE_SUBTRACT);
        glClearColor(.02f,.025f,.04f,1);glClearDepth(depth);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if(pass==LightRenderPass::Core){glEnable(GL_BLEND);glDepthMask(GL_FALSE);}
        CameraPose camera;camera.position=glm::vec3(linear*glm::vec4(lamp.position,1))+offset+glm::vec3(0,0,8);
        camera.forward=glm::vec3(0,0,-1);
        const auto before=State();
        renderer.Draw(lights,Matrix_Translate(offset.x,offset.y,offset.z)*linear,
                      View(camera),projection,camera.position,time,0,pass);
        Check(State()==before,"program, depth, blend and VAO restored after light pass");
        return Read(window);
    };
    for(const auto pass:{LightRenderPass::Core,LightRenderPass::Halo}) {
        const auto dark=render(pass,off,1,glm::vec3(0));
        const auto bright=render(pass,on,1,glm::vec3(0));
        Check(bright.Sum()>dark.Sum()+500,pass==LightRenderPass::Core?"GPU core becomes emissive ON":"GPU halo appears ON and disappears OFF");
        const auto blocked=render(pass,on,0,glm::vec3(0));
        glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT);const auto baseline=Read(window);
        Check(blocked.rgb==baseline.rgb,pass==LightRenderPass::Core?"opaque depth occludes core":"opaque depth occludes halo");
        if(pass==LightRenderPass::Halo)Check(dark.rgb==baseline.rgb,"OFF halo leaves the background unchanged");
        const auto distant=render(pass,on,1,glm::vec3(6000,700,6200));
        unsigned long long difference=0;
        for(std::size_t i=0;i<distant.rgb.size();++i)difference+=std::abs(int(distant.rgb[i])-int(bright.rgb[i]));
        Check(difference<2000,"translated/rotated/scaled light agrees at 6 km camera coordinates");
    }
    glClearDepth(1);glBlendEquation(GL_FUNC_ADD);
}
struct Shot {
    const char* name;int kind;bool off;
    Shot(const char* name,int kind,bool off=false):name(name),kind(kind),off(off) {}
};
// Front/back use LOOK_AT; detail views freeze a FREE camera; the cockpit uses
// the same controller and catalog eye as gameplay. Both meshes point along +X.
CameraPose ShotCamera(const Shot& shot,const AircraftSpec& spec,const RenderModel& model) {
    CameraFrame frame;frame.cockpitEyeLocal=spec.cockpitEye;
    frame.orbitTarget=(model.min+model.max)*.5f;frame.defaultOrbitDistance=spec.light?22.0f:65.0f;
    if(shot.kind==3) {
        frame.orbitTarget=spec.light?glm::vec3(2,2.7f,5.56f):glm::vec3(-11.7f,.7f,17.2f);
        frame.defaultOrbitDistance=spec.light?6.0f:10.0f;
    }
    if(shot.kind==4){frame.orbitTarget=glm::vec3(-7.3f,2.3f,0);frame.defaultOrbitDistance=8;}
    CameraController controller;controller.Reset(frame);
    if(shot.kind==0||shot.kind==4)controller.Look(3.14159265359f/.00060f,0);
    if(shot.kind==3)controller.Look((3.14159265359f*.5f)/.00060f,0);
    if(shot.kind>=3)controller.SetMode(CameraType::FREE,frame);
    if(shot.kind==2)controller.SetMode(CameraType::COCKPIT,frame);
    return controller.Pose(frame);
}
void Measure(const Pixels& pixels,const glm::mat4& viewProjection,const std::vector<aircraftlights::Light>& lights) {
    for(std::size_t i=0;i<lights.size();++i) {
        const auto& p=lights[i].position;const glm::vec4 clip=viewProjection*glm::vec4(p,1);
        if(clip.w<=0)continue;
        const glm::vec3 ndc=glm::vec3(clip)/clip.w;
        if(std::abs(ndc.x)>=1||std::abs(ndc.y)>=1||std::abs(ndc.z)>=1)continue;
        const int x=int((ndc.x*.5f+.5f)*pixels.width),y=int((ndc.y*.5f+.5f)*pixels.height);
        int peak=0;
        for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx) {
            const int xx=x+dx,yy=y+dy;if(xx<0||yy<0||xx>=pixels.width||yy>=pixels.height)continue;
            const auto index=(std::size_t(yy)*pixels.width+xx)*3;
            peak=std::max(peak,int(std::max(pixels.rgb[index],std::max(pixels.rgb[index+1],pixels.rgb[index+2]))));
        }
        std::printf("  anchor %zu local=(%.2f,%.2f,%.2f) framebuffer=(%d,%d) patch_peak=%d\n",i,p.x,p.y,p.z,x,y,peak);
    }
}
}

void InspectAircraftLights(GLFWwindow* window,GLuint sceneProgram,const std::string& prefix) {
    Check(window&&glfwGetCurrentContext()==window&&sceneProgram,"current real OpenGL context and scene shader");
    int width,height;glfwGetFramebufferSize(window,&width,&height);
    Check(width>0&&height>0,"nonempty real framebuffer");glViewport(0,0,width,height);
    glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    AircraftLightRenderer lightRenderer;ModelRenderer renderer;renderer.SetProgram(sceneProgram);
    GpuChecks(window,sceneProgram,lightRenderer,Matrix_Perspective(1.0471975512f,float(width)/height,-.05f,-20000));
    const Shot a320[]={{"a320-front-on",0},{"a320-front-off",0,true},{"a320-back",1},
                       {"a320-cockpit",2},{"a320-wing",3},{"a320-beacon",4}};
    const Shot light[]={{"light-front",0},{"light-back",1},{"light-cockpit",2},{"light-wing",3}};
    for(const auto& spec:AircraftCatalog()) {
        Pump(window);LoadCallbacks callbacks;
        callbacks.pump=[&](){Pump(window);glClearColor(.60f,.76f,.90f,1);glClear(GL_COLOR_BUFFER_BIT);glfwSwapBuffers(window);};
        RenderModel model=renderer.Load(spec.modelPath,false,nullptr,callbacks);
        try {
            const auto& lights=aircraftlights::Profile(spec.id);
            std::printf("Aircraft lights asset %s bounds: (%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)\n",
                spec.id.c_str(),model.min.x,model.min.y,model.min.z,model.max.x,model.max.y,model.max.z);
            const auto beacon=std::find_if(lights.begin(),lights.end(),[](const aircraftlights::Light& l){return l.pattern==aircraftlights::Pattern::Beacon;});
            Check(beacon!=lights.end(),spec.id+" has a beacon in its real-asset profile");
            const Shot* shots=spec.light?light:a320;const int count=spec.light?4:6;
            for(int i=0;i<count;++i) {
                Pump(window);const auto& shot=shots[i];const CameraPose camera=ShotCamera(shot,spec,model);
                const glm::mat4 view=View(camera),projection=Matrix_Perspective(camera.fieldOfView,float(width)/height,-camera.nearPlane,-20000);
                const double time=PatternTime(beacon->pattern,!shot.off,beacon->phaseOffset);
                glUseProgram(sceneProgram);glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));
                glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
                glClearColor(.60f,.76f,.90f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                renderer.Draw(model,glm::mat4(1),camera.position,RenderPass::Solid);
                lightRenderer.Draw(lights,glm::mat4(1),view,projection,camera.position,time,0,LightRenderPass::Core);
                glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
                renderer.Draw(model,glm::mat4(1),camera.position,RenderPass::Decal);
                renderer.Draw(model,glm::mat4(1),camera.position,RenderPass::Transparent);
                lightRenderer.Draw(lights,glm::mat4(1),view,projection,camera.position,time,0,LightRenderPass::Halo);
                const auto pixels=Read(window);
                std::size_t foreground=0;
                for(std::size_t p=0;p<pixels.rgb.size();p+=3)
                    if(std::abs(int(pixels.rgb[p])-153)+std::abs(int(pixels.rgb[p+1])-194)+std::abs(int(pixels.rgb[p+2])-230)>12)++foreground;
                Check(foreground>200,std::string(shot.name)+" contains the loaded aircraft geometry");
                Save(pixels,prefix+"-"+shot.name+".ppm");
                std::printf("  camera=(%.3f,%.3f,%.3f) time=%.3f beacon=%.3f\n",camera.position.x,camera.position.y,camera.position.z,time,
                    aircraftlights::Intensity(beacon->pattern,time,beacon->phaseOffset));
                Measure(pixels,projection*view,lights);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glfwSwapBuffers(window);
            }
        } catch(...) {renderer.Release(model);throw;}
        renderer.Release(model);
    }
    Check(glGetError()==GL_NO_ERROR,"ten real-asset captures and all GPU checks complete");
}
