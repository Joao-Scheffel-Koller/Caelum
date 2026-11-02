#include "render/GameScene.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <fstream>

// FONTE: codigo base do professor, include/matrices.h (definido em main.cpp).
// Forward declaration avoids duplicating the header's non-inline definitions.
glm::mat4 Matrix_Translate(float tx,float ty,float tz);

namespace {
mission::MissionSample MissionInput(const collision::ContactResult& contact,
                                    const std::vector<collision::ProbeSphere>& probes) {
    mission::MissionSample sample;
    sample.position=contact.position;sample.velocity=contact.velocity;sample.forward=contact.forward;
    sample.aircraftState=contact.state;sample.gearContacts=contact.gearContacts;
    for(const auto& probe:probes)if(probe.gear) {
        sample.gearCenters.push_back(probe.center);sample.gearRadii.push_back(probe.radius);
    }
    return sample;
}
}

void GameScene::Load(GLFWwindow* window) {
    aircraftModel=renderer.Load("../../data/aircraft/aircraft.obj");
    contacts.Configure(glm::dvec3(aircraftModel.min),glm::dvec3(aircraftModel.max));
    chunks=renderer.LoadMap("../../data/rio/chunks/manifest.json",[window](size_t loaded){
        std::string title="Caelum - carregando Rio ("+std::to_string(loaded)+" partes)";
        glfwSetWindowTitle(window,title.c_str());
        glfwPollEvents();
        if(glfwWindowShouldClose(window))throw std::runtime_error("Loading cancelled");
        glClearColor(.60f,.76f,.90f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    },&physicalWorld);
    for(const char* file:{"base_terrain.obj"}) {
        MapChunk repairs;
        repairs.file=file;
        // Finite support matching the visible repair surface. Real water at Y=0
        // is reached before this Y=-0.02 fallback and remains a water accident.
        repairs.model=renderer.Load(std::string("../../data/rio/chunks/")+file,true,&physicalWorld);
        repairs.min=repairs.model.min;repairs.max=repairs.model.max;
        repairs.foundation=true;
        chunks.push_back(repairs);
    }
    glfwSetWindowTitle(window,"Caelum - Rio de Janeiro");
    std::printf("Scene ready: A320 and %zu Rio chunks.\n",chunks.size());
    std::printf("Collision world: %zu physical triangles in %zu indexed chunks.\n",
                physicalWorld.TriangleCount(),physicalWorld.ChunkCount());
    std::fflush(stdout);
}

collision::AircraftPose GameScene::PhysicalPose(Aircraft& aircraft) {
    collision::AircraftPose pose;
    pose.position=aircraft.get_position();pose.forward=aircraft.get_forward();pose.up=aircraft.get_up();
    return pose;
}
void GameScene::ResolvePlayer(Aircraft& aircraft,const collision::AircraftPose& previous,double dt) {
    playerContact=contacts.Resolve(physicalWorld,previous,PhysicalPose(aircraft),glm::dvec3(aircraft.get_velocity()),dt);
    aircraft.set_position(glm::vec3(playerContact.position));
    aircraft.set_velocity(glm::vec3(playerContact.velocity));
    aircraft.set_forward(glm::vec3(playerContact.forward));aircraft.set_up(glm::vec3(playerContact.up));
    aircraft.set_right(glm::normalize(glm::cross(aircraft.get_forward(),aircraft.get_up())));
    if(playerContact.state==collision::AircraftState::Crashed) {
        aircraft.set_acceleration(glm::vec3(0));aircraft.set_throttle(0);
    }
}
void GameScene::ResetPlayer(Aircraft& aircraft) {
    Spawn(aircraft);contacts.Reset();ResolvePlayer(aircraft,PhysicalPose(aircraft),0);
}
std::vector<collision::ProbeSphere> GameScene::PlayerProbes(Aircraft& aircraft) const {
    return contacts.DebugSpheres(PhysicalPose(aircraft));
}
mission::MissionSample GameScene::PlayerMissionSample(Aircraft& aircraft) const {
    return MissionInput(playerContact,PlayerProbes(aircraft));
}
bool GameScene::InspectMission(Aircraft& aircraft,const std::string& report) const {
    // Developer-only deterministic replay. Uses actual runway coordinates,
    // loaded map geometry and the same contact resolver; never moves the player.
    std::ofstream output(report);
    if(!output)throw std::runtime_error("Cannot create mission report: "+report);
    bool passed=true;
    const auto check=[&](bool condition,const std::string& name) {
        output<<(condition?"PASS ":"FAIL ")<<name<<'\n';
        std::printf("Mission integration: %s %s\n",condition?"PASS":"FAIL",name.c_str());
        passed=passed&&condition;
    };
    for(const auto* airport:{&mission::SantosDumont(),&mission::Galeao()})for(const auto& runway:airport->runways) {
        for(double fraction:{.2,.5,.8}) {
            const auto center=runway.start*(1-fraction)+runway.end*fraction;
            const auto hit=physicalWorld.SweepSphere(center+glm::dvec3(0,5,0),center-glm::dvec3(0,2,0),.1);
            check(hit.hit&&hit.surface==collision::Surface::Ground&&hit.normal.y>.95&&std::abs(hit.point.y-center.y)<.005,
                  runway.id+" centerline matches the actual exported ground");
        }
    }
    const auto origin=PlayerMissionSample(aircraft);
    check(origin.aircraftState==collision::AircraftState::Grounded&&origin.gearContacts==3,
          "real player begins supported at Santos Dumont");
    for(const auto& runway:mission::Galeao().runways) {
        mission::FlightMission replay;
        replay.Update(origin,0);
        auto airborne=origin;
        airborne.aircraftState=collision::AircraftState::Flying;airborne.gearContacts=0;
        airborne.position.y+=20;airborne.velocity=origin.forward*60.0;
        for(auto& gear:airborne.gearCenters)gear.y+=20;
        replay.Update(airborne,0);
        for(int i=0;i<60;++i)replay.Update(airborne,1.0/60);
        check(replay.Snapshot().departed,"origin support and actual liftoff arm the mission");
        const auto center=(runway.start+runway.end)*.5;
        const auto direction=glm::normalize(runway.end-runway.start);
        auto approach=airborne;
        const auto offset=center+glm::dvec3(0,100,0)-approach.position;
        approach.position+=offset;approach.forward=direction;approach.velocity=direction*60.0;
        for(auto& gear:approach.gearCenters)gear+=offset;
        replay.Update(approach,1.0/60);
        check(replay.Snapshot().stage==mission::Stage::Approach,"arrival guidance uses the actual destination runway area");
        auto inclinedArrival=replay;
        const double pitch=-5.0*std::acos(-1.0)/180.0;
        collision::AircraftPose inclinedHigh,inclinedLow;
        inclinedHigh.position=center+glm::dvec3(0,4,0);
        inclinedHigh.forward=direction*std::cos(pitch)+glm::dvec3(0,std::sin(pitch),0);
        inclinedHigh.up=-direction*std::sin(pitch)+glm::dvec3(0,std::cos(pitch),0);
        inclinedLow=inclinedHigh;inclinedLow.position=center;
        collision::AircraftContacts inclinedContacts;
        inclinedContacts.Configure(glm::dvec3(aircraftModel.min),glm::dvec3(aircraftModel.max));
        const auto inclined=inclinedContacts.Resolve(physicalWorld,inclinedHigh,inclinedLow,
                                                   direction*3.0+glm::dvec3(0,-1,0),4.0);
        collision::AircraftPose inclinedSettled;
        inclinedSettled.position=inclined.position;inclinedSettled.forward=inclined.forward;inclinedSettled.up=inclined.up;
        check(inclined.state==collision::AircraftState::Grounded&&inclined.gearContacts==1&&inclined.position.y>1,
              runway.id+" safe inclined first-wheel touchdown has an elevated pivot");
        inclinedArrival.Update(MissionInput(inclined,inclinedContacts.DebugSpheres(inclinedSettled)),4.0);
        collision::AircraftPose high,low;
        high.position=center+glm::dvec3(0,1,0);high.forward=direction;
        low=high;low.position=center-glm::dvec3(0,.01,0);
        collision::AircraftContacts arrival;
        arrival.Configure(glm::dvec3(aircraftModel.min),glm::dvec3(aircraftModel.max));
        const auto contact=arrival.Resolve(physicalWorld,high,low,direction*3.0+glm::dvec3(0,-1,0),1.01);
        check(contact.state==collision::AircraftState::Grounded&&contact.gearContacts==3,
              runway.id+" supports the A320 landing gear at arrival");
        collision::AircraftPose settled;
        settled.position=contact.position;settled.forward=contact.forward;settled.up=contact.up;
        const auto sample=MissionInput(contact,arrival.DebugSpheres(settled));
        auto failedArrival=replay;
        collision::AircraftContacts impact;
        impact.Configure(glm::dvec3(aircraftModel.min),glm::dvec3(aircraftModel.max));
        const auto crash=impact.Resolve(physicalWorld,high,low,direction*3.0+glm::dvec3(0,-30,0),1.01);
        collision::AircraftPose stopped;stopped.position=crash.position;stopped.forward=crash.forward;stopped.up=crash.up;
        failedArrival.Update(MissionInput(crash,impact.DebugSpheres(stopped)),1.01);
        check(crash.state==collision::AircraftState::Crashed&&failedArrival.Snapshot().stage==mission::Stage::Failed,
              runway.id+" actual hard impact fails the mission");
        replay.Update(sample,1.01);
        check(replay.Snapshot().stage==mission::Stage::Landing&&replay.Snapshot().landingSeconds==0,
              "touchdown does not credit the previous airborne interval");
        for(int i=0;i<130;++i)replay.Update(sample,1.0/60);
        check(replay.Snapshot().stage==mission::Stage::Completed,
              runway.id+" completes after a stable safe arrival");
        inclinedArrival.Update(sample,1.0/60);
        for(int i=0;i<130;++i)inclinedArrival.Update(sample,1.0/60);
        check(inclinedArrival.Snapshot().stage==mission::Stage::Completed,
              runway.id+" completes after a safe one-wheel touchdown and leveling");
        mission::FlightMission skippedDeparture;
        for(int i=0;i<180;++i)skippedDeparture.Update(sample,1.0/60);
        check(skippedDeparture.Snapshot().stage!=mission::Stage::Completed,
              "being on the destination runway alone cannot complete the route");
    }
    output<<"RESULT "<<(passed?"PASS":"FAIL")<<'\n';output.flush();std::fflush(stdout);
    return passed&&bool(output);
}
bool GameScene::InspectCollisions(Aircraft& aircraft,const std::string& report) const {
    // Developer-only integration checks against the complete, actually loaded
    // Rio geometry. Local checker never replaces normal gameplay state/data.
    std::ofstream output(report);
    if(!output)throw std::runtime_error("Cannot create collision report: "+report);
    bool passed=true;
    const auto check=[&](bool condition,const char* name) {
        output<<(condition?"PASS ":"FAIL ")<<name<<'\n';
        std::printf("Collision inspection: %s %s\n",condition?"PASS":"FAIL",name);
        passed=passed&&condition;
    };
    collision::AircraftContacts checker;
    checker.Configure(glm::dvec3(aircraftModel.min),glm::dvec3(aircraftModel.max));
    const auto start=PhysicalPose(aircraft);
    auto result=checker.Resolve(physicalWorld,start,start,glm::dvec3(0),0);
    check(result.state==collision::AircraftState::Grounded,"Santos Dumont spawn supported by runway");
    check(result.gearContacts>=2,"multiple landing gear probes contact real asphalt");
    auto moved=start;moved.position+=start.forward*10.0;
    result=checker.Resolve(physicalWorld,start,moved,start.forward*25.0,.4);
    check(result.state==collision::AircraftState::Grounded&&glm::length(result.position-moved.position)<.1,
          "taxi advances without tangential ground lock");
    checker.Reset();
    moved=start;moved.position.y+=5;
    result=checker.Resolve(physicalWorld,start,moved,glm::dvec3(0,10,0),.5);
    check(result.state==collision::AircraftState::Flying,"upward departure releases ground support");
    checker.Reset();
    auto falling=start;falling.position.y=10;
    moved=start;moved.position.y=-2;
    result=checker.Resolve(physicalWorld,falling,moved,glm::dvec3(0,-30,0),.4);
    check(result.state==collision::AircraftState::Crashed&&result.reason==collision::CrashReason::HardLanding,
          "hard landing measured before any vertical clamp");
    result=checker.Resolve(physicalWorld,start,start,glm::dvec3(5,0,0),.1);
    check(result.state==collision::AircraftState::Crashed&&glm::length(result.velocity)==0,
          "accident remains stopped until reset");
    checker.Reset();result=checker.Resolve(physicalWorld,start,start,glm::dvec3(0),0);
    check(result.state==collision::AircraftState::Grounded,"reset restores normal runway support");
    for(auto surface:{collision::Surface::Water,collision::Surface::Solid}) {
        collision::Triangle triangle;
        const bool found=physicalWorld.SampleSurface(surface,triangle);
        check(found,surface==collision::Surface::Water?"water geometry retained":"solid obstacle geometry retained");
        if(found) {
            const auto center=(triangle.a+triangle.b+triangle.c)/3.0;
            std::size_t tested=0;
            const auto hit=physicalWorld.SweepSphere(center,center,.02,&tested);
            check(hit.hit&&hit.surface==surface,surface==collision::Surface::Water?
                  "actual water is not classified as fallback grass":"actual obstacle contact reaches solid geometry");
            check(tested<physicalWorld.TriangleCount()/10,"BVH rejects distant map geometry");
            output<<"candidate triangle tests: "<<tested<<" / "<<physicalWorld.TriangleCount()<<'\n';
        }
    }
    output<<"RESULT "<<(passed?"PASS":"FAIL")<<'\n';output.flush();std::fflush(stdout);
    return passed&&bool(output);
}

void GameScene::Spawn(Aircraft& aircraft) const {
    // Santos Dumont runway 20L: 100 m south of its northern OSM endpoint.
    // OSM endpoints: (-22.9045972,-43.163205), (-22.9164863,-43.1624422).
    // OSM2World origin: (-22.84895,-43.2142); meters, +Y up, north -Z.
    // The exported ASPHALT_0 triangles at this point have Y=0.
    aircraft.set_position(glm::vec3(5237.1933f,0.0f,6295.7127f));
    aircraft.set_forward(glm::normalize(glm::vec3(.05899537f,0,.99825826f)));
    aircraft.set_up(glm::vec3(0,1,0));
    aircraft.set_right(glm::normalize(glm::cross(aircraft.get_forward(),aircraft.get_up())));
    aircraft.set_velocity(glm::vec3(0));
    aircraft.set_acceleration(glm::vec3(0));
    aircraft.set_throttle(0);
}

glm::mat4 GameScene::AircraftTransform(Aircraft& aircraft) const {
    // Asset nose is +X, up is +Y; position denotes the wheel contact point.
    glm::mat4 orientation(1);
    glm::vec3 forward=glm::normalize(aircraft.get_forward());
    glm::vec3 right=glm::normalize(glm::cross(forward,aircraft.get_up()));
    orientation[0]=glm::vec4(forward,0);
    orientation[1]=glm::vec4(glm::normalize(glm::cross(right,forward)),0);
    orientation[2]=glm::vec4(right,0);
    glm::vec3 pivot=(aircraftModel.min+aircraftModel.max)*.5f;
    pivot.y=aircraftModel.min.y;
    const glm::vec3 position=aircraft.get_position();
    return Matrix_Translate(position.x,position.y,position.z)*orientation*
           Matrix_Translate(-pivot.x,-pivot.y,-pivot.z);
}

CameraFrame GameScene::Cameras(Aircraft& aircraft) const {
    CameraFrame frame;
    frame.aircraftModel=AircraftTransform(aircraft);
    frame.orbitTarget=aircraft.get_position()+glm::vec3(0,4,0);
    // OBJ survey: above the left seat back, below overhead, behind dashboard.
    frame.cockpitEyeLocal=glm::vec3(7.90f,.95f,-.65f);
    return frame;
}

void GameScene::Draw(GLuint program,Aircraft& aircraft,const glm::mat4& viewProjection,const glm::vec3& camera,SceneLayer layer) {
    renderer.SetProgram(program); // Also refreshes locations after R reloads shaders.
    glDisable(GL_CULL_FACE); // Trees/fences contain intentionally two-sided quads.
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    const glm::mat4 identity(1), transform=AircraftTransform(aircraft);
    const FlightPose npc=traffic.Pose();
    const glm::vec3 pivot=(aircraftModel.min+aircraftModel.max)*.5f;
    const glm::mat4 npcTransform=Matrix_Translate(npc.position.x,npc.position.y,npc.position.z)*
        npc.Orientation()*Matrix_Translate(-pivot.x,-pivot.y,-pivot.z);
    std::vector<const MapChunk*> visible;
    if(layer!=SceneLayer::AIRCRAFT)for(const auto& chunk:chunks)if(renderer.Visible(chunk,viewProjection)) {
        visible.push_back(&chunk);
        if(chunk.foundation) {glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.0f,4.0f);}
        renderer.Draw(chunk.model,identity,camera,RenderPass::Solid);
        if(chunk.foundation)glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if(layer!=SceneLayer::WORLD)renderer.Draw(aircraftModel,transform,camera,RenderPass::Solid);
    // Traffic belongs to the world even in cockpit; the near pass is player-only.
    if(layer!=SceneLayer::AIRCRAFT)renderer.Draw(aircraftModel,npcTransform,camera,RenderPass::Solid);

    // Draw all opaque geometry first, then glass from far to near.
    struct Instance {const RenderModel* model;glm::mat4 transform;glm::vec3 center;};
    std::vector<Instance> transparent;
    for(const auto* chunk:visible)
        transparent.push_back({&chunk->model,identity,(chunk->min+chunk->max)*.5f});
    if(layer!=SceneLayer::WORLD)transparent.push_back({&aircraftModel,transform,aircraft.get_position()});
    if(layer!=SceneLayer::AIRCRAFT)transparent.push_back({&aircraftModel,npcTransform,npc.position});
    std::sort(transparent.begin(),transparent.end(),[&](const Instance& a,const Instance& b){
        glm::vec3 da=a.center-camera,db=b.center-camera;
        return glm::dot(da,da)>glm::dot(db,db);
    });
    glEnable(GL_BLEND);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // FONTE: https://wikis.khronos.org/opengl/GLAPI/glPolygonOffset
    // Paint layers sit only 1mm above the base
    // in the OSM export; blend them after all solids with a small depth bias.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f,-2.0f);
    for(const auto& item:transparent)renderer.Draw(*item.model,item.transform,camera,RenderPass::Decal);
    glDisable(GL_POLYGON_OFFSET_FILL);
    for(const auto& item:transparent)renderer.Draw(*item.model,item.transform,camera,RenderPass::Transparent);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindSampler(0,0); // Text rendering owns its own texture state.
}

GameScene::~GameScene() {
    renderer.Release(aircraftModel);
    for(auto& chunk:chunks)renderer.Release(chunk.model);
}
