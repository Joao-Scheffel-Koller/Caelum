#include "render/GameScene.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

// FONTE: codigo base do professor, include/matrices.h (definido em main.cpp).
// Forward declaration avoids duplicating the header's non-inline definitions.
glm::mat4 Matrix_Translate(float tx,float ty,float tz);

void GameScene::Load(GLFWwindow* window) {
    aircraftModel=renderer.Load("../../data/aircraft/aircraft.obj");
    chunks=renderer.LoadMap("../../data/rio/chunks/manifest.json",[window](size_t loaded){
        std::string title="Caelum - carregando Rio ("+std::to_string(loaded)+" partes)";
        glfwSetWindowTitle(window,title.c_str());
        glfwPollEvents();
        if(glfwWindowShouldClose(window))throw std::runtime_error("Loading cancelled");
        glClearColor(.60f,.76f,.90f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    });
    for(const char* file:{"base_terrain.obj"}) {
        MapChunk repairs;
        repairs.file=file;
        repairs.model=renderer.Load(std::string("../../data/rio/chunks/")+file,true);
        repairs.min=repairs.model.min;repairs.max=repairs.model.max;
        repairs.foundation=true;
        chunks.push_back(repairs);
    }
    glfwSetWindowTitle(window,"Caelum - Rio de Janeiro");
    std::printf("Scene ready: A320 and %zu Rio chunks.\n",chunks.size());
    std::fflush(stdout);
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
