#include "collision/CollisionDebug.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// FONTE: shader compilation helpers supplied by the professor in main.cpp.
GLuint LoadShader_Vertex(const char*);
GLuint LoadShader_Fragment(const char*);
GLuint CreateGpuProgram(GLuint,GLuint);

CollisionDebug::CollisionDebug() {
    const auto vertex=LoadShader_Vertex("../../src/collision/shaders/debug_vertex.glsl");
    const auto fragment=LoadShader_Fragment("../../src/collision/shaders/debug_fragment.glsl");
    program=CreateGpuProgram(vertex,fragment);glDeleteShader(vertex);glDeleteShader(fragment);
    viewUniform=glGetUniformLocation(program,"view");
    projectionUniform=glGetUniformLocation(program,"projection");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
    glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
}
CollisionDebug::~CollisionDebug() {
    glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
}
void CollisionDebug::Draw(const std::vector<collision::ProbeSphere>& probes,const glm::mat4& view,
                          const glm::mat4& projection,const glm::vec3& camera) {
    std::vector<float> lines;
    for(const auto& sphere:probes)for(int axis=0;axis<3;++axis)for(int i=0;i<32;++i)for(int end=0;end<2;++end) {
        const double angle=2*std::acos(-1.0)*(i+end)/32;
        glm::dvec3 point=sphere.center-glm::dvec3(camera);
        point[(axis+1)%3]+=sphere.radius*std::cos(angle);
        point[(axis+2)%3]+=sphere.radius*std::sin(angle);
        for(int k=0;k<3;++k)lines.push_back(float(point[k]));
        lines.push_back(sphere.gear?1.0f:.1f);lines.push_back(sphere.gear?.75f:1.0f);lines.push_back(.2f);
    }
    GLint oldProgram,oldVao,oldBuffer;
    glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldBuffer);
    const bool depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND);
    glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glUseProgram(program);glUniformMatrix4fv(viewUniform,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projectionUniform,1,GL_FALSE,glm::value_ptr(projection));
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,lines.size()*sizeof(float),lines.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_LINES,0,GLsizei(lines.size()/6));
    glUseProgram(oldProgram);glBindVertexArray(oldVao);glBindBuffer(GL_ARRAY_BUFFER,oldBuffer);
    if(depth)glEnable(GL_DEPTH_TEST);if(blend)glEnable(GL_BLEND);
}
