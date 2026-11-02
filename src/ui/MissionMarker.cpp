#include "ui/MissionMarker.hpp"
#include "mission/Airports.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <string>

// FONTE: shader loading helpers and font renderer from the professor's base
// code, src/main.cpp and src/ui/textrendering.cpp. Billboard geometry is new.
GLuint LoadShader_Vertex(const char*);
GLuint LoadShader_Fragment(const char*);
GLuint CreateGpuProgram(GLuint,GLuint);
void TextRendering_PrintString(GLFWwindow*,const std::string&,float,float,float);
float TextRendering_CharWidth(GLFWwindow*);

namespace {
void Enabled(GLenum capability,bool enabled) {if(enabled)glEnable(capability);else glDisable(capability);}
// The marker and the existing font renderer must leave the scene/HUD state
// intact, including when the cockpit has just rendered its separate depth pass.
struct State {
    GLint program,vao,buffer,depthFunc,polygon[2],srcRgb,dstRgb,srcAlpha,dstAlpha,eqRgb,eqAlpha;
    GLboolean depthWrite;
    bool depth,blend,cull,coverage,scissor;
    State():depth(glIsEnabled(GL_DEPTH_TEST)),blend(glIsEnabled(GL_BLEND)),
        cull(glIsEnabled(GL_CULL_FACE)),coverage(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE)),
        scissor(glIsEnabled(GL_SCISSOR_TEST)) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
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

MissionMarker::MissionMarker() {
    const auto& runways=mission::Galeao().runways;
    for(const auto& runway:runways)anchor+=(runway.start+runway.end)*.5;
    anchor/=double(runways.size());anchor.y+=450;
    const GLuint vertex=LoadShader_Vertex("../../src/ui/shaders/mission_marker_vertex.glsl");
    const GLuint fragment=LoadShader_Fragment("../../src/ui/shaders/mission_marker_fragment.glsl");
    program=CreateGpuProgram(vertex,fragment);glDeleteShader(vertex);glDeleteShader(fragment);
    anchorUniform=glGetUniformLocation(program,"clip_anchor");
    sizeUniform=glGetUniformLocation(program,"size_ndc");
    const float quad[]={-1,-1,1,-1,1,1,-1,-1,1,1,-1,1};
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
}
MissionMarker::~MissionMarker() {
    glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
}
void MissionMarker::Draw(GLFWwindow* window,const glm::mat4& view,const glm::mat4& projection,
                         const glm::vec3& camera) const {
    int width,height;glfwGetWindowSize(window,&width,&height);
    if(width<=0||height<=0)return;
    // Subtract the camera once, before conversion to float, matching the map's
    // camera-relative rendering. Only the rotation part of view remains here.
    const auto relative=glm::vec3(anchor-glm::dvec3(camera));
    const auto clip=projection*glm::vec4(glm::mat3(view)*relative,1);
    for(int i=0;i<4;++i)if(!std::isfinite(clip[i]))return;
    if(clip.w<=0||clip.z < -clip.w||clip.z>clip.w)return;
    const float size=std::min(144.0f,height*.24f);
    State state;
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glUseProgram(program);glUniform4f(anchorUniform,clip.x,clip.y,clip.z,clip.w);
    glUniform2f(sizeUniform,size*.6f/width,size/height);
    glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,6);
    const std::string label="GALEAO";
    const float textScale=1.05f;
    const float x=clip.x/clip.w-TextRendering_CharWidth(window)*textScale*label.size()*.5f;
    TextRendering_PrintString(window,label,x,clip.y/clip.w-1.1f*size/height,textScale);
}
