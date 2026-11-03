#ifndef CAELUM_MODEL_RENDERER_HPP
#define CAELUM_MODEL_RENDERER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "render/MaterialPolicy.hpp"
#include "platform/LoadProgress.hpp"
namespace collision {class CollisionWorld;}

enum class RenderPass { Solid, Decal, Transparent };

// AI-assisted renderer for OBJ/MTL assets. Coordinates are never normalized.
struct RenderMaterial {
    glm::vec3 diffuse{0.8f};
    float opacity=1.0f;
    GLuint texture=0;
    bool clamp=false;
    MaterialPolicy policy;
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
    bool foundation=false;
};

class ModelRenderer {
public:
    ~ModelRenderer();
    RenderModel Load(const std::string& path,bool map=false,collision::CollisionWorld* physicalWorld=nullptr,
                     const LoadCallbacks& callbacks={});
    std::vector<MapChunk> LoadMap(const std::string& manifest,
        const std::function<void(size_t)>& progress = {},collision::CollisionWorld* physicalWorld=nullptr,
        const LoadCallbacks& callbacks={});
    static std::size_t CountMapChunks(const std::string& manifest);
    // Call after loading/reloading the shader program (R remains supported).
    void SetProgram(GLuint program);
    void Draw(const RenderModel& model, const glm::mat4& transform,
              const glm::vec3& camera, RenderPass pass);
    bool Visible(const MapChunk& chunk,const glm::mat4& viewProjection) const;
    void Release(RenderModel& model);
private:
    std::map<std::string,GLuint> textures;
    GLuint samplers[2]={0,0};
    GLint modelUniform=-1, colorUniform=-1, opacityUniform=-1,
          textureUniform=-1, hasTextureUniform=-1, cameraUniform=-1,
          proceduralUniform=-1, alphaCutoffUniform=-1;
    GLuint LoadTexture(const std::string& file,const LoadCallbacks& callbacks);
};
#endif
