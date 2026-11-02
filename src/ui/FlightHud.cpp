#include "ui/FlightHud.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

// FONTE: shader loading helpers and font renderer from the professor's base code.
// Their implementations remain in src/main.cpp and src/ui/textrendering.cpp.
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
GLuint CreateGpuProgram(GLuint vertex,GLuint fragment);
void TextRendering_PrintString(GLFWwindow*,const std::string&,float,float,float);
float TextRendering_CharWidth(GLFWwindow*);

namespace {
const double Pi=3.14159265358979323846;
double Radians(double degrees) {return degrees*Pi/180;}
void Enabled(GLenum capability,bool enabled) {if(enabled)glEnable(capability);else glDisable(capability);}
// Text rendering has its own program/VAO and changes depth/blend/polygon modes.
// Preserve all state changed by the complete HUD pass, including those helpers.
struct State {
    GLint program,vao,buffer,depthFunc,polygon[2],srcRgb,dstRgb,srcAlpha,dstAlpha,eqRgb,eqAlpha;
    GLboolean depthWrite;
    bool depth,blend,cull,coverage,scissor;
    State():depth(glIsEnabled(GL_DEPTH_TEST)),blend(glIsEnabled(GL_BLEND)),
        cull(glIsEnabled(GL_CULL_FACE)),coverage(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE)),
        scissor(glIsEnabled(GL_SCISSOR_TEST)) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqAlpha);
    }
    ~State() {
        glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glDepthFunc(depthFunc);glDepthMask(depthWrite);glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
        glBlendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha);glBlendEquationSeparate(eqRgb,eqAlpha);
        Enabled(GL_DEPTH_TEST,depth);Enabled(GL_BLEND,blend);Enabled(GL_CULL_FACE,cull);
        Enabled(GL_SAMPLE_ALPHA_TO_COVERAGE,coverage);Enabled(GL_SCISSOR_TEST,scissor);
    }
};
}
FlightHud::FlightHud() {
    const GLuint vertex=LoadShader_Vertex("../../src/ui/shaders/hud_vertex.glsl");
    const GLuint fragment=LoadShader_Fragment("../../src/ui/shaders/hud_fragment.glsl");
    program=CreateGpuProgram(vertex,fragment);
    glDeleteShader(vertex);glDeleteShader(fragment);
    resolutionUniform=glGetUniformLocation(program,"resolution");
    attitudeUniform=glGetUniformLocation(program,"attitude");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    const GLsizei stride=sizeof(Vertex);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(Vertex,x)));
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(Vertex,r)));
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(Vertex,u)));
    glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<void*>(offsetof(Vertex,mode)));
    for(GLuint i=0;i<4;++i)glEnableVertexAttribArray(i);
    glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
    vertices.reserve(10000);labels.reserve(100);
}
FlightHud::~FlightHud() {
    glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
}
void FlightHud::Triangle(float ax,float ay,float bx,float by,float cx,float cy,Color c) {
    for(const auto& p:{glm::vec2(ax,ay),glm::vec2(bx,by),glm::vec2(cx,cy)})
        vertices.push_back({p.x,p.y,c.r,c.g,c.b,c.a,0,0,0});
}
void FlightHud::Line(float ax,float ay,float bx,float by,float thickness,Color c) {
    const float dx=bx-ax,dy=by-ay,length=std::sqrt(dx*dx+dy*dy);
    if(length<1e-6f)return;
    const float x=-dy/length*thickness*.5f,y=dx/length*thickness*.5f;
    Triangle(ax+x,ay+y,bx+x,by+y,bx-x,by-y,c);
    Triangle(ax+x,ay+y,bx-x,by-y,ax-x,ay-y,c);
}
void FlightHud::Disc(float x,float y,float radius,Color c) {
    for(int i=0;i<96;++i) {
        const double a=2*Pi*i/96,b=2*Pi*(i+1)/96;
        Triangle(x,y,x+radius*std::cos(a),y+radius*std::sin(a),
                 x+radius*std::cos(b),y+radius*std::sin(b),c);
    }
}
void FlightHud::Ring(float x,float y,float radius,float thickness,Color c) {
    for(int i=0;i<96;++i) {
        const double a=2*Pi*i/96,b=2*Pi*(i+1)/96;
        const float ax=x+radius*std::cos(a),ay=y+radius*std::sin(a);
        const float bx=x+radius*std::cos(b),by=y+radius*std::sin(b);
        Line(ax,ay,bx,by,thickness,c);
    }
}
void FlightHud::Text(float x,float baseline,const std::string& text,float scale) {
    labels.push_back({x,baseline,std::max(.82f,scale),text});
}
void FlightHud::Needle(const HudDial& d,double angle,bool valid) {
    if(!valid)return;
    const float dx=std::cos(angle),dy=std::sin(angle),r=d.radius;
    const Color needle={1,.76f,.2f,1};
    Triangle(d.x-dy*r*.045f,d.y+dx*r*.045f,d.x+dx*r*.69f,d.y+dy*r*.69f,
             d.x+dy*r*.045f,d.y-dx*r*.045f,needle);
    Line(d.x,d.y,d.x-dx*r*.16f,d.y-dy*r*.16f,r*.04f,needle);
    Disc(d.x,d.y,r*.065f,{.8f,.83f,.86f,1});
}
void FlightHud::Gauge(const HudDial& d,const FlightTelemetry& t,float scale) {
    const float x=d.x,y=d.y,r=d.radius,s=scale;
    const Color ticks={.82f,.86f,.9f,1},frame={.35f,.40f,.45f,1};
    Disc(x,y,r,{.035f,.045f,.055f,.95f});
    Ring(x,y,r*.97f,std::max(1.0f,2*s),frame);
    if(d.instrument==FlightInstrument::Attitude) {
        const float inner=r*.87f;
        if(t.attitudeValid) {
            for(const auto& uv:{glm::vec2(-1,-1),glm::vec2(-1,1),glm::vec2(1,1),
                               glm::vec2(-1,-1),glm::vec2(1,1),glm::vec2(1,-1)})
                vertices.push_back({x+uv.x*inner,y-uv.y*inner,1,1,1,1,uv.x,uv.y,1});
            const double bank=Radians(t.rollDegrees),cs=std::cos(bank),sn=std::sin(bank);
            for(int pitch=-60;pitch<=60;pitch+=10) {
                if(pitch==0)continue;
                const double v=(pitch-t.pitchDegrees)/45.0;
                if(std::abs(v)>.77)continue;
                const double half=std::min(pitch%20==0?.36:.23,std::sqrt(.64-v*v));
                const auto project=[&](double u){return glm::vec2(x+inner*(cs*u-sn*v),y-inner*(sn*u+cs*v));};
                const auto a=project(-half),b=project(half);
                Line(a.x,a.y,b.x,b.y,std::max(1.0f,s),ticks);
            }
        } else Disc(x,y,inner,{.14f,.15f,.16f,1});
        Ring(x,y,inner,std::max(1.0f,s),frame);
        const Color marker={1,.76f,.2f,1};
        Line(x-r*.55f,y,x-r*.17f,y,2.5f*s,marker);
        Line(x+r*.17f,y,x+r*.55f,y,2.5f*s,marker);
        Line(x-r*.17f,y,x-r*.08f,y+r*.09f,2.5f*s,marker);
        Line(x+r*.08f,y+r*.09f,x+r*.17f,y,2.5f*s,marker);
        Disc(x,y,r*.035f,marker);
        char value[64];
        if(t.attitudeValid)std::snprintf(value,sizeof(value),"P%+.0f R%+.0f",t.pitchDegrees,t.rollDegrees);
        else std::snprintf(value,sizeof(value),"--");
        Text(x,y+r*.57f,"HORIZONTE",.82f*s);
        Text(x,y+r*.79f,value,.8f*s);
        return;
    }
    if(d.instrument==FlightInstrument::Compass) {
        for(int degrees=0;degrees<360;degrees+=10) {
            const double a=Radians(degrees-90);
            const float outer=r*.84f,inner=r*(degrees%30==0?.72f:.78f);
            Line(x+inner*std::cos(a),y+inner*std::sin(a),x+outer*std::cos(a),y+outer*std::sin(a),s,ticks);
        }
        const char* names[]={"N","L","S","O"};
        for(int i=0;i<4;++i) {
            const double a=Radians(i*90-90);
            Text(x+r*.62f*std::cos(a),y+r*.62f*std::sin(a)+4*s,names[i],.88f*s);
        }
        if(t.headingValid&&needleReadings.headingValid)
            Needle(d,Radians(HudNeedleDegrees(d.instrument,needleReadings.headingDegrees)-90),true);
        Text(x,y-r*.23f,"RUMO",.8f*s);
        char value[32];
        if(t.headingValid)std::snprintf(value,sizeof(value),"%03.0f",std::fmod(std::round(t.headingDegrees),360.0));
        else std::snprintf(value,sizeof(value),"--");
        Text(x,y+r*.3f,value,1.05f*s);
        return;
    }
    const bool speed=d.instrument==FlightInstrument::Speed,altitude=d.instrument==FlightInstrument::Altitude;
    const double maximum=speed?1000:altitude?1000:100;
    const double value=speed?t.speedKmh:altitude?t.altitudeMeters:t.throttlePercent;
    const bool valid=speed?t.speedValid:altitude?t.altitudeValid:t.throttleValid;
    for(int i=0;i<(altitude?20:21);++i) {
        const double a=Radians(HudNeedleDegrees(d.instrument,maximum*i/20)-90);
        const float outer=r*.85f,inner=r*(i%4==0?.72f:.79f);
        Line(x+inner*std::cos(a),y+inner*std::sin(a),x+outer*std::cos(a),y+outer*std::sin(a),s,ticks);
        if(i%4==0 && r>=52) {
            char number[16];std::snprintf(number,sizeof(number),"%.0f",maximum*i/20);
            Text(x+r*.58f*std::cos(a),y+r*.58f*std::sin(a)+3*s,number,.76f*s);
        }
    }
    const double needle=speed?needleReadings.speedKmh:altitude?needleReadings.altitudeMeters:
                                                               needleReadings.throttlePercent;
    const bool needleValid=speed?needleReadings.speedValid:altitude?needleReadings.altitudeValid:
                                                                  needleReadings.throttleValid;
    if(valid&&needleValid)Needle(d,Radians(HudNeedleDegrees(d.instrument,needle)-90),true);
    Text(x,y+r*.26f,speed?"VELOCIDADE":altitude?"ALTITUDE":"POTENCIA",.83f*s);
    char number[48];
    const char* unit=speed?"km/h":altitude?"m":"%";
    if(valid)std::snprintf(number,sizeof(number),"%.0f %s",value,unit);
    else std::snprintf(number,sizeof(number),"-- %s",unit);
    Text(x,y+r*.73f,number,1.02f*s);
}
void FlightHud::Update(const FlightTelemetry& telemetry,double elapsedSeconds) {
    needleReadings=needleMotion.Update(telemetry,elapsedSeconds);
}
void FlightHud::Draw(GLFWwindow* window,const FlightTelemetry& telemetry) {
    int width,height;glfwGetWindowSize(window,&width,&height);
    const auto layout=CalculateHudLayout(width,height);
    if(layout.dials.empty())return;
    vertices.clear();labels.clear();
    for(const auto& dial:layout.dials)Gauge(dial,telemetry,layout.scale);
    State state;
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glUseProgram(program);
    glUniform2f(resolutionUniform,float(width),float(height));
    glUniform2f(attitudeUniform,float(Radians(telemetry.pitchDegrees)),float(Radians(telemetry.rollDegrees)));
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,GLsizei(vertices.size()));
    const float charWidth=TextRendering_CharWidth(window);
    for(const auto& label:labels) {
        const float ndcX=2*label.x/width-1-charWidth*label.scale*label.text.size()*.5f;
        TextRendering_PrintString(window,label.text,ndcX,1-2*label.y/height,label.scale);
    }
}
