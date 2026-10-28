#ifndef CAELUM_GAME_SCENE_HPP
#define CAELUM_GAME_SCENE_HPP
#include "ModelRenderer.hpp"
#include "Aircraft.hpp"

// AI-assisted integration of the aircraft and georeferenced Rio chunks.
class GameScene {
public:
    ~GameScene();
    void Load(GLFWwindow* window);
    void Spawn(Aircraft& aircraft) const;
    void Draw(GLuint program, Aircraft& aircraft, const glm::mat4& viewProjection,
              const glm::vec3& camera);
private:
    ModelRenderer renderer;
    RenderModel aircraftModel;
    std::vector<MapChunk> chunks;
    glm::mat4 AircraftTransform(Aircraft& aircraft) const;
};
#endif
