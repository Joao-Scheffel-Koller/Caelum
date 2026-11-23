// AI-assisted: checks that data/pilot loads with the game's OBJ loader
// (tinyobj) and keeps the texture, UVs, size and pose documented in
// data/pilot/PROVENANCE.md. Usage: pilot_asset_test <path to data/pilot>
#include <tiny_obj_loader.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2){std::fprintf(stderr,"usage: pilot_asset_test <data/pilot>\n");return EXIT_FAILURE;}
    const std::string folder=std::string(argv[1])+"/";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    const bool loaded=tinyobj::LoadObj(&attrib,&shapes,&materials,&warning,&error,
                                       (folder+"pilot.obj").c_str(),folder.c_str(),true);
    Check(loaded&&error.empty(),"pilot.obj and pilot.mtl load");

    // Texture recovered from the GLB (Blender's OBJ export had dropped it).
    Check(materials.size()==1,"one material");
    Check(materials[0].diffuse_texname=="pilot_diffuse.jpg","material has map_Kd pilot_diffuse.jpg");
    Check(std::ifstream(folder+materials[0].diffuse_texname,std::ios::binary).good(),"texture file exists next to the MTL");
    Check(materials[0].diffuse[0]>.99f,"white Kd: the texture colour is not darkened");

    // Every triangle has a UV and a normal, all UVs inside the texture.
    size_t triangles=0;
    for(const auto& shape:shapes) {
        for(const auto& index:shape.mesh.indices) {
            Check(index.texcoord_index>=0,"every vertex has a UV");
            Check(index.normal_index>=0,"every vertex has a normal");
        }
        triangles+=shape.mesh.indices.size()/3;
    }
    Check(triangles>=700&&triangles<=2000,"low-poly mesh (cheap to draw in every seat)");
    for(float uv:attrib.texcoords)Check(uv>=0&&uv<=1,"UVs inside the texture");

    // Seated pose in metres: ~1.43 m tall, facing +Z, feet at Y~0.
    float lo[3]={1e9f,1e9f,1e9f},hi[3]={-1e9f,-1e9f,-1e9f};
    for(size_t i=0;i<attrib.vertices.size();i+=3)
        for(int a=0;a<3;++a){lo[a]=std::min(lo[a],attrib.vertices[i+a]);hi[a]=std::max(hi[a],attrib.vertices[i+a]);}
    Check(hi[1]-lo[1]>1.3f&&hi[1]-lo[1]<1.6f,"seated height ~1.43 m (units are metres)");
    Check(std::abs(lo[1])<.05f,"feet at Y~0");
    Check(hi[2]>2*-lo[2],"knees and hands point to +Z (the pilot faces +Z)");

    std::printf("pilot_asset: %d checks passed (%zu triangles)\n",checks,triangles);
    return EXIT_SUCCESS;
}
