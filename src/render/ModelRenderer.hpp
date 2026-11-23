#ifndef CAELUM_MODEL_RENDERER_HPP
#define CAELUM_MODEL_RENDERER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "render/MaterialPolicy.hpp"
#include "render/WindSway.hpp"
#include "platform/LoadProgress.hpp"
namespace collision {class CollisionWorld;}
namespace tinyobj {struct material_t;}

enum class RenderPass { Solid, Decal, Transparent };

// AI-assisted renderer for OBJ/MTL assets. Coordinates are never normalized.
struct RenderMaterial {
    glm::vec3 diffuse{0.8f};
    float opacity=1.0f;
    GLuint texture=0;
    bool clamp=false;
    MaterialPolicy policy;
};
struct RenderVertex {glm::vec3 position,normal;glm::vec2 uv;};
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
    // AI-assisted (docs/propeller.md): loads an OBJ as several models in one
    // read. The objects ("o" lines) named in pieces[i] go to parts[i]; the rest
    // is returned. Each model keeps its own bounds. Throws if a name is missing.
    RenderModel LoadSplit(const std::string& path,const std::vector<std::vector<std::string>>& pieces,
                          std::vector<RenderModel>& parts,const LoadCallbacks& callbacks={});
    // Two-sided flat disc of the given radius in the YZ plane, centred at the origin.
    RenderModel CreateDisc(float radius,int segments,const glm::vec3& color);
    static std::size_t CountMapChunks(const std::string& manifest);
    // Call after loading/reloading the shader program (R remains supported).
    void SetProgram(GLuint program);
    // depthBias: extra depth-priority steps for the whole model (Solid pass),
    // added to each material's DepthPriority (e.g. the low filler terrain).
    // alpha multiplies every material's opacity; a batch whose resulting
    // opacity is below 1 belongs to the Transparent pass (fading propeller).
    void Draw(const RenderModel& model, const glm::mat4& transform,
              const glm::vec3& camera, RenderPass pass, int depthBias=0, float alpha=1.0f);
    bool Visible(const MapChunk& chunk,const glm::mat4& viewProjection) const;
    // Wind state used by the next Draw calls (sway materials only).
    void SetWind(const WindParameters& parameters,float time) {wind=parameters;windTime=time;}
    void Release(RenderModel& model);
private:
    std::map<std::string,GLuint> textures;
    GLuint samplers[2]={0,0};
    GLint modelUniform=-1, colorUniform=-1, opacityUniform=-1,
          textureUniform=-1, hasTextureUniform=-1, cameraUniform=-1,
          proceduralUniform=-1, alphaCutoffUniform=-1,
          windStrengthUniform=-1, windTimeUniform=-1, windDirectionUniform=-1,
          windAmplitudeUniform=-1, windFrequencyUniform=-1, windWavenumberUniform=-1,
          windFlutterUniform=-1, windFlutterFrequencyUniform=-1;
    WindParameters wind;
    float windTime=0;
    GLuint LoadTexture(const std::string& file,const LoadCallbacks& callbacks);
    void Upload(std::map<int,std::vector<RenderVertex>>& grouped,const std::vector<tinyobj::material_t>& materials,
                const std::string& base,bool map,const LoadCallbacks& callbacks,RenderModel& result);
    void UploadBatch(const std::vector<RenderVertex>& vertices,RenderBatch& batch);
};
#endif
