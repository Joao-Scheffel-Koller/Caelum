#include <tiny_obj_loader.h>
#include <sstream>
#include <map>
#include <vector>
#include <cmath>
#include <iostream>

int main() {
    // A colored material must not prevent the next texture-only material from
    // getting the loader's diffuse default (the aircraft tail/panels hit this).
    std::istringstream source(
        "newmtl paint\nKd .2 .4 .6\n"
        "newmtl tail\nmap_Kd logo.png\n"
        "newmtl real_black\nKd 0 0 0\nmap_Kd black.png\n"
        "newmtl cockpit\nmap_Kd panel.png\n");
    std::map<std::string,int> indices;
    std::vector<tinyobj::material_t> materials;
    std::string warning,error;
    tinyobj::LoadMtl(&indices,&materials,&source,&warning,&error);
    if(materials.size()!=4)return 1;
    for(const char* name:{"tail","cockpit"})for(int component=0;component<3;++component)
        if(std::abs(materials[indices[name]].diffuse[component]-.6f)>.0001f) {
            std::cerr<<"FAIL: inherited Kd state darkens "<<name<<'\n';return 1;
        }
    for(int component=0;component<3;++component)
        if(materials[indices["real_black"]].diffuse[component]!=0)return 1;
    if(std::abs(materials[indices["paint"]].diffuse[0]-.2f)>.0001f)return 1;
    std::cout<<"PASS: texture defaults isolated per material; explicit colors preserved\n";
}
