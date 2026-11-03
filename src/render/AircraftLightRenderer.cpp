#include "render/AircraftLightRenderer.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

// FONTE: shader loading/linking helpers supplied by the professor in src/main.cpp.
// The light meshes, procedural lens texture and billboard shaders are new.
GLuint LoadShader_Vertex(const char*);
GLuint LoadShader_Fragment(const char*);
GLuint CreateGpuProgram(GLuint,GLuint);

namespace {
void Enabled(GLenum capability,bool enabled) {
    if(enabled)glEnable(capability);else glDisable(capability);
}

// Both initialization and drawing must preserve the surrounding scene's state.
struct State {
    GLint program,vao,buffer,depthFunc,polygon[2],cullMode,frontFace;
    GLint srcRgb,dstRgb,srcAlpha,dstAlpha,eqRgb,eqAlpha;
    GLboolean depthWrite,sampleInvert;
    GLfloat sampleValue;
    bool depth,blend,cull,alphaCoverage,alphaToOne,sampleCoverage,polygonOffset;

    State():depth(glIsEnabled(GL_DEPTH_TEST)),blend(glIsEnabled(GL_BLEND)),
        cull(glIsEnabled(GL_CULL_FACE)),alphaCoverage(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE)),
        alphaToOne(glIsEnabled(GL_SAMPLE_ALPHA_TO_ONE)),sampleCoverage(glIsEnabled(GL_SAMPLE_COVERAGE)),
        polygonOffset(glIsEnabled(GL_POLYGON_OFFSET_FILL)) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_CULL_FACE_MODE,&cullMode);glGetIntegerv(GL_FRONT_FACE,&frontFace);
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqAlpha);
        glGetFloatv(GL_SAMPLE_COVERAGE_VALUE,&sampleValue);
        glGetBooleanv(GL_SAMPLE_COVERAGE_INVERT,&sampleInvert);
    }

    ~State() {
        glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glDepthFunc(depthFunc);glDepthMask(depthWrite);
        // OpenGL core stores one shared polygon mode for front and back faces.
        glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
        glCullFace(cullMode);glFrontFace(frontFace);
        glBlendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha);glBlendEquationSeparate(eqRgb,eqAlpha);
        glSampleCoverage(sampleValue,sampleInvert);
        Enabled(GL_DEPTH_TEST,depth);Enabled(GL_BLEND,blend);Enabled(GL_CULL_FACE,cull);
        Enabled(GL_SAMPLE_ALPHA_TO_COVERAGE,alphaCoverage);Enabled(GL_SAMPLE_ALPHA_TO_ONE,alphaToOne);
        Enabled(GL_SAMPLE_COVERAGE,sampleCoverage);Enabled(GL_POLYGON_OFFSET_FILL,polygonOffset);
    }
};

struct Vertex {glm::vec3 position,normal;};

glm::vec3 SpherePoint(int latitude,int longitude) {
    const float pi=3.14159265358979323846f;
    const float elevation=-pi*.5f+pi*latitude/6.0f,azimuth=2.0f*pi*longitude/8.0f;
    return glm::vec3(std::cos(elevation)*std::cos(azimuth),std::sin(elevation),
                     std::cos(elevation)*std::sin(azimuth));
}

void Triangle(std::vector<Vertex>& vertices,const glm::vec3& a,const glm::vec3& b,const glm::vec3& c) {
    vertices.push_back({a,a});vertices.push_back({b,b});vertices.push_back({c,c});
}

bool Finite(const glm::vec3& value) {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}
}

AircraftLightRenderer::~AircraftLightRenderer() {
    if(vbo)glDeleteBuffers(1,&vbo);
    if(vao)glDeleteVertexArrays(1,&vao);
    if(program)glDeleteProgram(program);
}

void AircraftLightRenderer::Initialize() {
    if(program)return;
    std::vector<Vertex> vertices;
    vertices.reserve(246);
    for(int latitude=0;latitude<6;++latitude)for(int longitude=0;longitude<8;++longitude) {
        const auto a=SpherePoint(latitude,longitude),b=SpherePoint(latitude+1,longitude);
        const auto c=SpherePoint(latitude+1,longitude+1),d=SpherePoint(latitude,longitude+1);
        if(latitude<5)Triangle(vertices,a,b,c);
        if(latitude>0)Triangle(vertices,a,c,d);
    }
    coreCount=GLsizei(vertices.size());
    const glm::vec3 normal(0,0,1);
    for(const auto& point:{glm::vec3(-1,-1,0),glm::vec3(1,-1,0),glm::vec3(1,1,0),
                          glm::vec3(-1,-1,0),glm::vec3(1,1,0),glm::vec3(-1,1,0)})
        vertices.push_back({point,normal});

    const GLuint vertex=LoadShader_Vertex("../../src/render/shaders/aircraft_light_vertex.glsl");
    const GLuint fragment=LoadShader_Fragment("../../src/render/shaders/aircraft_light_fragment.glsl");
    // CreateGpuProgram already marks both shader objects for deletion.
    program=CreateGpuProgram(vertex,fragment);
    GLint linked=GL_FALSE;glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE) {
        glDeleteProgram(program);program=0;
        throw std::runtime_error("Cannot link aircraft light shaders");
    }
    viewUniform=glGetUniformLocation(program,"view_rotation");
    projectionUniform=glGetUniformLocation(program,"projection");
    linearUniform=glGetUniformLocation(program,"model_linear");
    normalUniform=glGetUniformLocation(program,"model_normal");
    centerUniform=glGetUniformLocation(program,"center_relative");
    radiusUniform=glGetUniformLocation(program,"light_radius");
    colorUniform=glGetUniformLocation(program,"light_color");
    intensityUniform=glGetUniformLocation(program,"light_intensity");
    passUniform=glGetUniformLocation(program,"light_pass");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,position)));
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
}

void AircraftLightRenderer::Draw(const std::vector<aircraftlights::Light>& lights,const glm::mat4& model,
                                 const glm::mat4& view,const glm::mat4& projection,const glm::vec3& camera,
                                 double time,double phaseOffset,LightRenderPass pass) {
    if(lights.empty())return;
    const glm::mat3 linear(model),rotation(view);
    const float scale=std::max(glm::length(linear[0]),std::max(glm::length(linear[1]),glm::length(linear[2])));
    if(!std::isfinite(scale)||scale<=0||!Finite(camera)||std::abs(glm::determinant(linear))<1e-12f)return;
    const glm::mat3 normal=glm::transpose(glm::inverse(linear));
    State state;
    Initialize();
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glDepthMask(pass==LightRenderPass::Core?GL_TRUE:GL_FALSE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    glDisable(GL_SAMPLE_COVERAGE);glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    if(pass==LightRenderPass::Core) {
        glDisable(GL_BLEND);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);
        glFrontFace(glm::determinant(linear)<0?GL_CW:GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE,GL_ZERO,GL_ONE);
    }
    glUseProgram(program);glBindVertexArray(vao);
    glUniformMatrix3fv(viewUniform,1,GL_FALSE,glm::value_ptr(rotation));
    glUniformMatrix4fv(projectionUniform,1,GL_FALSE,glm::value_ptr(projection));
    glUniformMatrix3fv(linearUniform,1,GL_FALSE,glm::value_ptr(linear));
    glUniformMatrix3fv(normalUniform,1,GL_FALSE,glm::value_ptr(normal));
    glUniform1i(passUniform,pass==LightRenderPass::Core?0:1);
    const glm::dmat4 preciseModel(model);
    for(const auto& light:lights) {
        const float intensity=aircraftlights::Intensity(light.pattern,time,phaseOffset+light.phaseOffset);
        if(pass==LightRenderPass::Halo&&intensity<=.001f)continue;
        const float radius=pass==LightRenderPass::Core?light.radius:light.haloRadius*scale;
        if(!std::isfinite(radius)||radius<=0||!Finite(light.position)||!Finite(light.color))continue;
        // Preserve small local anchors at Rio coordinates: subtract the camera
        // in double precision before uploading a camera-relative float center.
        const glm::vec3 center(glm::dvec3(preciseModel*glm::dvec4(light.position,1))-glm::dvec3(camera));
        if(!Finite(center))continue;
        const glm::vec4 clip=projection*glm::vec4(rotation*center,1);
        // Keep the near-plane intersection of the physical mesh valid. The GPU
        // clips geometry; there is no distance-based minimum pixel enlargement.
        if(!std::isfinite(clip.w)||clip.w<=0)continue;
        glUniform3fv(centerUniform,1,glm::value_ptr(center));
        glUniform1f(radiusUniform,radius);
        glUniform3fv(colorUniform,1,glm::value_ptr(light.color));
        glUniform1f(intensityUniform,intensity);
        glDrawArrays(GL_TRIANGLES,pass==LightRenderPass::Core?0:coreCount,
                     pass==LightRenderPass::Core?coreCount:6);
    }
}
