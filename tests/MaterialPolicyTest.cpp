// AI-assisted CPU regression checks using material names from the real assets.
#include "render/MaterialPolicy.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int checks=0;
void Check(bool condition,const char* message) {
    ++checks;
    if(!condition) {std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
void Image(const char* name,const char* path,MaterialLayer layer) {
    const auto policy=ClassifyMaterial(name,path,true,true);
    Check(policy.layer==layer,name);
    Check(policy.procedural==ProceduralSurface::NONE,"image material keeps supplied texture");
}
}

int main() {
    Image("ASPHALT_0","textures/afba57240769_Asphalt010_Color.png",MaterialLayer::BASE);
    Image("ROAD_MARKING_DASHED_0","textures/afba57240769_Asphalt010_Color.png",MaterialLayer::BASE);
    Image("ROAD_MARKING_DASHED_1","textures/23c218772c21_road_marking_dash.png",MaterialLayer::DECAL);
    Image("MAT_13_0","textures/afba57240769_Asphalt010_Color.png",MaterialLayer::BASE);
    Image("MAT_13_1","textures/3a0c7ad66a72_runway_centerline_stripes.png",MaterialLayer::DECAL);
    Image("MAT_16_1","textures/3a0c7ad66a72_runway_centerline_stripes.png",MaterialLayer::DECAL);
    Image("MAT_0_1","textures/7b0e4d51fbd4_road_marking_crossing.png",MaterialLayer::DECAL);
    Image("MAT_20_1","textures/e81b24d05414_tex-helipad_markings-15298634225582335805.png",MaterialLayer::DECAL);
    Image("MAT_23_1","textures/7e2f09469e1b_Windows_Color.png",MaterialLayer::DECAL);
    Image("PITCH_SOCCER_0","textures/066b3d2a2f4e_Grass-2048.JPG",MaterialLayer::BASE);
    Image("PITCH_SOCCER_1","textures/6b27129d98fe_pitchSoccer.png",MaterialLayer::DECAL);
    Image("PITCH_TENNIS_CLAY_1","textures/c0cfe76c79ac_pitchTennis.png",MaterialLayer::DECAL);
    Image("PITCH_BEACHVOLLEYBALL_1","textures/2e6d0c57306d_pitchBeachvolleyball.png",MaterialLayer::DECAL);
    Image("TREE_BILLBOARD_BROAD_LEAVED_0","textures/311ded243f5d_arbaro_tree_broad_leaved.png",MaterialLayer::BASE);
    Image("TREE_BILLBOARD_CONIFEROUS_0","textures/bcd526a81eba_arbaro_tree_coniferous.png",MaterialLayer::BASE);
    Image("CHAIN_LINK_FENCE_0","textures/1e9ecd31ca79_Fence006_Color.png",MaterialLayer::BASE);
    Image("POWER_TOWER_VERTICAL_0","textures/ee3cc833bce4_powertower_vertical.png",MaterialLayer::BASE);
    Image("TENNIS_NET_0","textures/eda115b410a8_tennis_net.png",MaterialLayer::BASE);
    Image("GLASS_0","textures/817138bb6971_Glass_Color.jpg",MaterialLayer::BASE);
    Image("WATER_0","textures/79e4126a535c_P1140080E_Water.jpg",MaterialLayer::BASE);
    Image("BUILDING_DEFAULT_0","textures/c63b921b3612_Plaster002_Color.jpg",MaterialLayer::BASE);
    Image("MAT_99_2","textures/overlay-alpha.png",MaterialLayer::DECAL);
    Image("MAT_99_1","textures/tree_billboard.png",MaterialLayer::BASE);

    const auto paint=ClassifyMaterial("TAXIWAY_CENTER_MARKING_0","",false,true);
    Check(paint.layer==MaterialLayer::DECAL,"plain yellow taxiway paint is a decal");
    Check(paint.procedural==ProceduralSurface::PAINT,"plain yellow paint receives procedural detail");
    const auto fence=ClassifyMaterial("METAL_FENCE_0","",false,true);
    Check(fence.layer==MaterialLayer::BASE,"physical metal fence is not a decal");
    Check(fence.procedural==ProceduralSurface::METAL,"metal receives procedural fallback");
    Check(ClassifyMaterial("grass","",false,true).procedural==ProceduralSurface::GRASS,"unmapped grass fallback");
    Check(ClassifyMaterial("Exterior_white","",false,false).procedural==ProceduralSurface::GENERAL,"plain aircraft exterior fallback");
    Check(ClassifyMaterial("metal_1","",false,false).procedural==ProceduralSurface::METAL,"aircraft metal fallback");
    Check(ClassifyMaterial("black_leather","",false,false).procedural==ProceduralSurface::GENERAL,"aircraft leather fallback");
    Check(ClassifyMaterial("","",false,false).procedural==ProceduralSurface::GENERAL,"unknown plain material fallback");

    // Map naming rules cannot accidentally reclassify aircraft meshes.
    for(const char* name:{"Exterior_corrected","MAT_13_1","TAXIWAY_CENTER_MARKING_0"}) {
        const auto aircraft=ClassifyMaterial(name,"textures/runway_centerline_stripes.png",true,false);
        Check(aircraft.layer==MaterialLayer::BASE,"aircraft images remain base geometry");
        Check(aircraft.procedural==ProceduralSurface::NONE,"aircraft mapped texture remains unchanged");
    }
    const auto lower=ClassifyMaterial("mat_13_1","TEXTURES/RUNWAY_CENTERLINE_STRIPES.PNG",true,true);
    Check(lower.layer==MaterialLayer::DECAL,"classification handles case consistently");
    // AI-assisted: coplanar layers measured in the export (same Y=0) get an
    // ordered depth priority: natural ground < pavement < specific surfaces.
    const auto depth=[](const char* name,const char* image){
        return ClassifyMaterial(name,image,image[0]!=0,true).depth;
    };
    Check(depth("GRASS_0","textures/Ground003_Color.jpg")==DepthPriority::NATURAL_GROUND,"grass is natural ground");
    Check(depth("WATER_0","")==DepthPriority::NATURAL_GROUND,"water lies under roads crossing it");
    Check(depth("SCRUB_0","")==DepthPriority::NATURAL_GROUND,"scrub is natural ground");
    Check(depth("ASPHALT_0","textures/Asphalt010_Color.png")==DepthPriority::PAVEMENT,"asphalt is pavement");
    Check(depth("CONCRETE_0","")==DepthPriority::PAVEMENT,"concrete is pavement");
    Check(depth("GRASS_PAVER_0","")==DepthPriority::PAVEMENT,"grass paver is pavement, not grass");
    Check(depth("MAT_13_0","textures/Asphalt010_Color.png")==DepthPriority::SURFACE,"runway base wins over apron asphalt");
    Check(depth("RED_ROAD_MARKING_0","")==DepthPriority::SURFACE,"bike lane wins over asphalt");
    Check(depth("ROAD_MARKING_DASHED_0","textures/Asphalt010_Color.png")==DepthPriority::SURFACE,"lane base wins over asphalt");
    Check(depth("SANDSTONE_0","")==DepthPriority::SURFACE,"sandstone walls are not ground");
    Check(depth("ROOF_DEFAULT_0","")==DepthPriority::SURFACE,"roofs keep the default depth");
    Check(ClassifyMaterial("grass","",false,false).depth==DepthPriority::SURFACE,"aircraft materials are never offset");
    Check(depth("TAXIWAY_CENTER_MARKING_0","")==DepthPriority::SURFACE,"decals keep their own pass offset");
    std::cout<<"PASS: "<<checks<<" material policy checks\n";
    return EXIT_SUCCESS;
}
