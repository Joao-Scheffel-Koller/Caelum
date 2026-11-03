#ifndef CAELUM_AIRCRAFT_LIGHT_RENDERER_HPP
#define CAELUM_AIRCRAFT_LIGHT_RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "aircraft/AircraftLights.hpp"

enum class LightRenderPass { Core, Halo };

// GPU resources are allocated on the first nonempty Draw, with a current context.
// Destroy this renderer before destroying that context.
class AircraftLightRenderer {
public:
    AircraftLightRenderer() = default;
    ~AircraftLightRenderer();
    AircraftLightRenderer(const AircraftLightRenderer&) = delete;
    AircraftLightRenderer& operator=(const AircraftLightRenderer&) = delete;

    // model is the complete aircraft transform, including local alignment/scale.
    // Core belongs before transparent glass; Halo belongs after transparent glass.
    void Draw(const std::vector<aircraftlights::Light>& lights,const glm::mat4& model,
              const glm::mat4& view,const glm::mat4& projection,const glm::vec3& camera,
              double time,double phaseOffset,LightRenderPass pass);

private:
    void Initialize();
    GLuint program=0,vao=0,vbo=0;
    GLsizei coreCount=0;
    GLint viewUniform=-1,projectionUniform=-1,linearUniform=-1,normalUniform=-1,
          centerUniform=-1,radiusUniform=-1,colorUniform=-1,intensityUniform=-1,
          passUniform=-1;
};

#endif
