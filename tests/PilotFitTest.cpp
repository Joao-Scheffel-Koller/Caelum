// AI-assisted: the seated pilots fit both cabins, measured on the real meshes
// (docs/pilot.md). For every pilot vertex, a ray goes sideways from the centre
// of his own seat to the vertex: if it crosses the aircraft first, that vertex
// sticks out through a window or wall (outer arm) or into the centre console
// (inner arm).
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

// Loads an OBJ; objects whose name starts with an entry of skip are ignored.
Mesh Load(const std::string& folder,const std::string& file,const std::vector<std::string>& skip={}) {
    tinyobj::attrib_t attrib;std::vector<tinyobj::shape_t> shapes;std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    Check(tinyobj::LoadObj(&attrib,&shapes,&materials,&warning,&error,(folder+file).c_str(),folder.c_str(),true),
          "OBJ loads");
    Mesh mesh;
    for(size_t i=0;i<attrib.vertices.size();i+=3)
        mesh.vertices.emplace_back(attrib.vertices[i],attrib.vertices[i+1],attrib.vertices[i+2]);
    for(const auto& shape:shapes) {
        bool skipped=false;
        for(const auto& prefix:skip)skipped=skipped||shape.name.compare(0,prefix.size(),prefix)==0;
        if(skipped)continue;
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
// How far (m) the deepest vertex crosses the aircraft, rays from the seat centre.
float Poke(const std::vector<glm::vec3>& pilot,const std::vector<Triangle>& skin,float seatZ) {
    float worst=0;
    for(const auto& p:pilot) {
        const float offset=p.z-seatZ,distance=std::abs(offset);
        if(distance<1e-6f)continue;
        const glm::vec3 origin(p.x,p.y,seatZ),direction(0,0,offset>0?1.0f:-1.0f);
        for(const auto& t:skin) {
            const float h=Hit(origin,direction,t);
            if(h>0&&h<distance)worst=std::max(worst,distance-h);
        }
    }
    return worst;
}

struct Cabin {
    const char* id;const char* folder;std::vector<std::string> skip;
    float x0,x1,y0,y1;          // Cabin region (only nearby triangles are tested).
};

void CheckCabin(const std::string& data,const Mesh& pilotMesh,const Cabin& cabin) {
    const Mesh aircraft=Load(data+cabin.folder,"aircraft.obj",cabin.skip);
    std::vector<Triangle> skin;
    for(const auto& t:aircraft.triangles) {
        const float x0=std::min({t.a.x,t.b.x,t.c.x}),x1=std::max({t.a.x,t.b.x,t.c.x});
        const float y0=std::min({t.a.y,t.b.y,t.c.y}),y1=std::max({t.a.y,t.b.y,t.c.y});
        if(x1>=cabin.x0&&x0<=cabin.x1&&y1>=cabin.y0&&y0<=cabin.y1)skin.push_back(t);
    }
    Check(!skin.empty(),"cabin geometry found");
    const AircraftSpec* spec=nullptr;
    for(const auto& s:AircraftCatalog())if(s.id==cabin.id)spec=&s;
    Check(spec&&spec->crew.seats.size()==2,"aircraft has two seats");
    std::vector<std::vector<glm::vec3>> placed;
    for(const auto& seat:spec->crew.seats) {
        const glm::mat4 m=pilot::SeatMatrix(seat,spec->crew.scale);
        std::vector<glm::vec3> points;
        for(const auto& v:pilotMesh.vertices)points.push_back(glm::vec3(m*glm::vec4(v,1.0f)));
        const float poke=Poke(points,skin,seat.z);
        std::printf("%s seat z=%+.3f: deepest vertex crossing the cabin %.1f cm\n",cabin.id,seat.z,poke*100);
        Check(poke<.005f,"no part of the pilot crosses windows, walls or the centre console");
        placed.push_back(points);
    }
    // Real gap between the two pilots' meshes: the inner half of each one
    // (the side facing the other seat: inner arms and shoulders).
    float gap=1e9f;
    for(const auto& a:placed[0])if(a.z>spec->crew.seats[0].z)
        for(const auto& b:placed[1])if(b.z<spec->crew.seats[1].z)gap=std::min(gap,glm::length(a-b));
    std::printf("%s gap between the pilots: %.1f cm\n",cabin.id,gap*100);
    Check(gap>.02f,"the two pilots do not touch (gap > 2 cm)");
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2){std::fprintf(stderr,"usage: pilot_fit_test <data>\n");return EXIT_FAILURE;}
    const std::string data=std::string(argv[1])+"/";
    const Mesh pilotMesh=Load(data+"pilot/","pilot.obj");
    // Light aircraft: seats, floor and panel are not walls.
    CheckCabin(data,pilotMesh,{"light","light_aircraft/",{"SeatBase","SeatBack","CabinFloor","CabinDashboard"},
                               1.4f,3.4f,1.3f,2.9f});
    // A320: the seats, and the narrow column under the panel at each seat's
    // centre line, which passes between the pilot's knees (docs/pilot.md).
    CheckCabin(data,pilotMesh,{"a320","aircraft/",{"a_chair","dash-8_single.001","dash-8_single.005"},
                               7.2f,8.9f,-.4f,1.7f});
    std::printf("pilot_fit: %d checks passed\n",checks);
    return EXIT_SUCCESS;
}
