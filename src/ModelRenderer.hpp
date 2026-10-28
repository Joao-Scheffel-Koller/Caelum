#ifndef CAELUM_MODEL_RENDERER_HPP
#define CAELUM_MODEL_RENDERER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

// AI-assisted renderer for OBJ/MTL assets. Coordinates are never normalized.
struct RenderMaterial {
    glm::vec3 diffuse{0.8f};
    float opacity=1.0f;
    GLuint texture=0;
    bool clamp=false;
};
struct RenderBatch {
    GLuint vao=0, vbo=0;
    GLsizei count=0;
    RenderMaterial material;
    glm::vec3 center{0};
};
struct RenderModel {
    std::vector<RenderBatch> batches;
    glm::vec3 min, max;
};
struct MapChunk {
    std::string file;
    glm::vec3 min, max;
    RenderModel model;
};

class ModelRenderer {
public:
    ~ModelRenderer();
    RenderModel Load(const std::string& path);
    std::vector<MapChunk> LoadMap(const std::string& manifest);
    // Call after loading/reloading the shader program (R remains supported).
    void SetProgram(GLuint program);
    void Draw(const RenderModel& model, const glm::mat4& transform,
              const glm::vec3& camera, bool transparent);
    bool Visible(const MapChunk& chunk,const glm::mat4& viewProjection) const;
    void Release(RenderModel& model);
private:
    std::map<std::string,GLuint> textures;
    GLuint samplers[2]={0,0};
    GLint modelUniform=-1, colorUniform=-1, opacityUniform=-1,
          textureUniform=-1, hasTextureUniform=-1;
    GLuint LoadTexture(const std::string& file);
};
#endif
