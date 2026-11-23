// AI-assisted: the seated pilots fit the light aircraft's cabin, measured on
// the real meshes (docs/pilot.md). For every pilot vertex, a ray goes sideways
// from the cabin centre line (Z=0) to the vertex: if it crosses the aircraft
// skin first, that vertex sticks out through a window or wall.
// Usage: pilot_fit_test <path to data>
#include "aircraft/AircraftCatalog.hpp"
#include "aircraft/PilotCrew.hpp"
#include "matrices.h"
#include <tiny_obj_loader.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
struct Triangle {glm::vec3 a,b,c;};
struct Mesh {std::vector<glm::vec3> vertices;std::vector<Triangle> triangles;};

// Loads an OBJ; objects named in skip (seats, floor, panel) are not skin.
Mesh Load(const std::string& folder,const std::string& file,const std::vector<std::string>& skip={}) {
    tinyobj::attrib_t attrib;std::vector<tinyobj::shape_t> shapes;std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    Check(tinyobj::LoadObj(&attrib,&shapes,&materials,&warning,&error,(folder+file).c_str(),folder.c_str(),true),
          "OBJ loads");
    Mesh mesh;
    for(size_t i=0;i<attrib.vertices.size();i+=3)
        mesh.vertices.emplace_back(attrib.vertices[i],attrib.vertices[i+1],attrib.vertices[i+2]);
    for(const auto& shape:shapes) {
        if(std::find(skip.begin(),skip.end(),shape.name)!=skip.end())continue;
        for(size_t i=0;i+2<shape.mesh.indices.size();i+=3)
            mesh.triangles.push_back({mesh.vertices[shape.mesh.indices[i].vertex_index],
                                      mesh.vertices[shape.mesh.indices[i+1].vertex_index],
                                      mesh.vertices[shape.mesh.indices[i+2].vertex_index]});
    }
    return mesh;
}
// Moller-Trumbore: distance along the ray, or -1 when it misses.
float Hit(const glm::vec3& origin,const glm::vec3& direction,const Triangle& t) {
    const glm::vec3 e1=t.b-t.a,e2=t.c-t.a,p=glm::cross(direction,e2);
    const float det=glm::dot(e1,p);
    if(std::abs(det)<1e-12f)return -1;
    const glm::vec3 s=origin-t.a;
    const float u=glm::dot(s,p)/det;if(u<0||u>1)return -1;
    const glm::vec3 q=glm::cross(s,e1);
    const float v=glm::dot(direction,q)/det;if(v<0||u+v>1)return -1;
    return glm::dot(e2,q)/det;
}
// How far (m) the deepest vertex sticks out through the skin.
float Poke(const std::vector<glm::vec3>& pilot,const std::vector<Triangle>& skin) {
    float worst=0;
    for(const auto& p:pilot) {
        const glm::vec3 origin(p.x,p.y,0),direction(0,0,p.z>0?1.0f:-1.0f);
        const float distance=std::abs(p.z);
        for(const auto& t:skin) {
            const float h=Hit(origin,direction,t);
            if(h>0&&h<distance)worst=std::max(worst,distance-h);
        }
    }
    return worst;
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2){std::fprintf(stderr,"usage: pilot_fit_test <data>\n");return EXIT_FAILURE;}
    const std::string data=std::string(argv[1])+"/";
    const Mesh pilotMesh=Load(data+"pilot/","pilot.obj");
    const Mesh aircraft=Load(data+"light_aircraft/","aircraft.obj",
                             {"SeatBase","SeatBack","CabinFloor","CabinDashboard"});
    std::vector<Triangle> skin;   // Cabin region only (speed).
    for(const auto& t:aircraft.triangles) {
        const float x0=std::min({t.a.x,t.b.x,t.c.x}),x1=std::max({t.a.x,t.b.x,t.c.x});
        const float y0=std::min({t.a.y,t.b.y,t.c.y}),y1=std::max({t.a.y,t.b.y,t.c.y});
        if(x1>=1.4f&&x0<=3.4f&&y1>=1.3f&&y0<=2.9f)skin.push_back(t);
    }
    Check(!skin.empty(),"cabin skin found");

    const AircraftSpec* light=nullptr;
    for(const auto& spec:AircraftCatalog())if(spec.id=="light")light=&spec;
    Check(light&&light->crew.seats.size()==2,"light aircraft has two seats");
    std::vector<std::vector<glm::vec3>> placed;
    for(const auto& seat:light->crew.seats) {
        const glm::mat4 m=pilot::SeatMatrix(seat,light->crew.scale);
        std::vector<glm::vec3> points;
        for(const auto& v:pilotMesh.vertices)points.push_back(glm::vec3(m*glm::vec4(v,1.0f)));
        const float poke=Poke(points,skin);
        std::printf("seat z=%+.3f: deepest vertex outside the skin %.1f cm\n",seat.z,poke*100);
        Check(poke<.005f,"no part of the pilot crosses the cabin skin (windows, walls)");
        placed.push_back(points);
    }
    // Real gap between the two pilots' meshes (inner arms and shoulders).
    float gap=1e9f;
    for(const auto& a:placed[0])if(a.z>-.25f)
        for(const auto& b:placed[1])if(b.z<.25f)gap=std::min(gap,glm::length(a-b));
    std::printf("gap between the pilots: %.1f cm\n",gap*100);
    Check(gap>.02f,"the two pilots do not touch (gap > 2 cm)");
    std::printf("pilot_fit: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
