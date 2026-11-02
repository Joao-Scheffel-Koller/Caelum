#ifndef CAELUM_GAME_SCENE_HPP
#define CAELUM_GAME_SCENE_HPP
#include "render/ModelRenderer.hpp"
#include "aircraft/Aircraft.hpp"
#include "camera/CameraController.hpp"
#include "aircraft/BezierFlight.hpp"
#include "collision/CollisionWorld.hpp"
#include "collision/AircraftContacts.hpp"
#include "mission/FlightMission.hpp"

enum class SceneLayer { ALL, WORLD, AIRCRAFT };

// AI-assisted integration of the aircraft and georeferenced Rio chunks.
class GameScene {
public:
    ~GameScene();
    void Load(GLFWwindow* window);
    void Spawn(Aircraft& aircraft) const;
    void ResolvePlayer(Aircraft& aircraft,const collision::AircraftPose& previous,double dt);
    void ResetPlayer(Aircraft& aircraft);
    const collision::ContactResult& PlayerContact() const {return playerContact;}
    std::vector<collision::ProbeSphere> PlayerProbes(Aircraft& aircraft) const;
    const collision::CollisionWorld& PhysicalWorld() const {return physicalWorld;}
    bool InspectCollisions(Aircraft& aircraft,const std::string& report) const;
    mission::MissionSample PlayerMissionSample(Aircraft& aircraft) const;
    bool InspectMission(Aircraft& aircraft,const std::string& report) const;
    static collision::AircraftPose PhysicalPose(Aircraft& aircraft);
    void Update(double dt) {traffic.Update(dt);}
    FlightPose TrafficPose() const {return traffic.Pose();}
    CameraFrame Cameras(Aircraft& aircraft) const;
    void Draw(GLuint program, Aircraft& aircraft, const glm::mat4& viewProjection,
              const glm::vec3& camera, SceneLayer layer=SceneLayer::ALL);
private:
    ModelRenderer renderer;
    RenderModel aircraftModel;
    BezierFlight traffic;
    std::vector<MapChunk> chunks;
    collision::CollisionWorld physicalWorld;
    collision::AircraftContacts contacts;
    collision::ContactResult playerContact;
    glm::mat4 AircraftTransform(Aircraft& aircraft) const;
};
#endif
