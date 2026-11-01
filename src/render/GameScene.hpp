#ifndef CAELUM_GAME_SCENE_HPP
#define CAELUM_GAME_SCENE_HPP
#include "render/ModelRenderer.hpp"
#include "aircraft/Aircraft.hpp"
#include "camera/CameraController.hpp"

enum class SceneLayer { ALL, WORLD, AIRCRAFT };

// AI-assisted integration of the aircraft and georeferenced Rio chunks.
class GameScene {
public:
    ~GameScene();
    void Load(GLFWwindow* window);
    void Spawn(Aircraft& aircraft) const;
    CameraFrame Cameras(Aircraft& aircraft) const;
    void Draw(GLuint program, Aircraft& aircraft, const glm::mat4& viewProjection,
              const glm::vec3& camera, SceneLayer layer=SceneLayer::ALL);
private:
    ModelRenderer renderer;
    RenderModel aircraftModel;
    std::vector<MapChunk> chunks;
    glm::mat4 AircraftTransform(Aircraft& aircraft) const;
};
#endif
