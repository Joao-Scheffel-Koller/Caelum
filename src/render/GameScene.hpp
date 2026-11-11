#ifndef CAELUM_GAME_SCENE_HPP
#define CAELUM_GAME_SCENE_HPP
#include "render/ModelRenderer.hpp"
#include "aircraft/Aircraft.hpp"
#include "camera/CameraController.hpp"
#include "aircraft/BezierFlight.hpp"
#include "collision/CollisionWorld.hpp"
#include "collision/AircraftContacts.hpp"
#include "mission/FlightMission.hpp"
#include "aircraft/AircraftLights.hpp"
#include "render/AircraftLightRenderer.hpp"

enum class SceneLayer { ALL, WORLD, AIRCRAFT };

// AI-assisted integration of the aircraft and georeferenced Rio chunks.
class GameScene {
public:
    ~GameScene();
    void Load(GLFWwindow* window,const LoadCallbacks& callbacks={},
              const std::string& aircraftPath="../../data/aircraft/aircraft.obj",
              const glm::vec3& cockpitEye=glm::vec3(7.90f,.95f,-.65f));
    void LoadAircraft(const std::string& aircraftPath,const glm::vec3& cockpitEye,
                      const LoadCallbacks& callbacks={});
    void Spawn(Aircraft& aircraft) const;
    void SetAirframeType(collision::AirframeType type);
    void SetLightProfile(const std::string& aircraftId);
    void ResolvePlayer(Aircraft& aircraft,const collision::AircraftPose& previous,double dt);
    void ResetPlayer(Aircraft& aircraft);
    const collision::ContactResult& PlayerContact() const {return playerContact;}
    std::vector<collision::ProbeSphere> PlayerProbes(Aircraft& aircraft) const;
    const collision::CollisionWorld& PhysicalWorld() const {return physicalWorld;}
    bool InspectCollisions(Aircraft& aircraft,const std::string& report) const;
    mission::MissionSample PlayerMissionSample(Aircraft& aircraft) const;
    bool InspectMission(Aircraft& aircraft,const std::string& report) const;
    static collision::AircraftPose PhysicalPose(Aircraft& aircraft);
    void Update(double dt) {traffic.Update(dt);lightClock.Update(dt);windClock.Update(dt);}
    FlightPose TrafficPose() const {return traffic.Pose();}
    CameraFrame Cameras(Aircraft& aircraft) const;
    void Draw(GLuint program, Aircraft& aircraft, const glm::mat4& view,const glm::mat4& projection,
              const glm::vec3& camera, SceneLayer layer=SceneLayer::ALL);
private:
    ModelRenderer renderer;
    RenderModel aircraftModel;
    BezierFlight traffic;
    AircraftLightRenderer lightRenderer;
    aircraftlights::Clock lightClock;
    WindClock windClock;      // AI-assisted: time-based tree sway (docs/wind.md).
    WindParameters wind;
    std::string lightProfileId="a320";
    std::vector<MapChunk> chunks;
    collision::CollisionWorld physicalWorld;
    collision::AircraftContacts contacts;
    collision::AirframeType airframeType=collision::AirframeType::Airliner;
    collision::ContactResult playerContact;
    glm::vec3 cockpitEyeLocal{7.90f,.95f,-.65f};
    glm::mat4 AircraftTransform(Aircraft& aircraft) const;
};
#endif
