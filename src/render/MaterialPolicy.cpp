#include "render/MaterialPolicy.hpp"
#include <algorithm>
#include <cctype>

namespace {
std::string Lower(std::string value) {
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
bool Contains(const std::string& text,const char* token) {
    return text.find(token)!=std::string::npos;
}
bool EndsWith(const std::string& text,const char* suffix) {
    const std::string ending(suffix);
    return text.size()>=ending.size() && text.compare(text.size()-ending.size(),ending.size(),ending)==0;
}
bool IsCutoutGeometry(const std::string& name,const std::string& path) {
    // These alpha images describe real geometry, not paint over another surface.
    for(const char* token:{"tree","billboard","fence","tennis_net","powertower","power_tower"})
        if(Contains(name,token)||Contains(path,token))return true;
    return false;
}
// Measured overlaps in the export (same Y): asphalt over grass and water,
// concrete/sett over grass, runway/lane/bike-lane surfaces over asphalt.
DepthPriority GroundPriority(const std::string& name) {
    for(const char* token:{"grass_paver","asphalt","concrete","sett","paving_stone",
                          "pebblestone","cobblestone","gravel"})
        if(Contains(name,token))return DepthPriority::PAVEMENT;
    if(Contains(name,"sandstone"))return DepthPriority::SURFACE;   // Walls, not ground.
    for(const char* token:{"grass","scrub","earth","sand","scree","water","wetland","meadow"})
        if(Contains(name,token))return DepthPriority::NATURAL_GROUND;
    // MAT_<n>_0 (runway/pitch bases), *_MARKING_0 lanes, roofs, railways...
    return DepthPriority::SURFACE;
}

bool IsOverlayImage(const std::string& path) {
    // Verified image families in this OSM2World export: road/airfield paint,
    // pitch markings and window layers drawn over their separately exported base.
    for(const char* token:{"road_marking","road_arrow","runway_centerline",
                          "airstripmarking","airstrip_marking","helipad_marking",
                          "pitchsoccer","pitchtennis","pitchbeachvolleyball","windows_color"})
        if(Contains(path,token))return true;
    return false;
}
}

MaterialPolicy ClassifyMaterial(const std::string& originalName,const std::string& diffusePath,
                               bool hasImage,bool isMap) {
    const std::string name=Lower(originalName),path=Lower(diffusePath);
    MaterialPolicy policy;
    // Tree billboards: texture v runs from 0 (base) to 1 (top), used as height.
    policy.sway=isMap && (Contains(name,"tree")||Contains(path,"tree"));
    if(isMap && !IsCutoutGeometry(name,path)) {
        // OSM2World names generated texture layers MAT_<material>_<layer>.
        // All observed layer 1 images are alpha overlays; layer 0 remains base.
        const bool generatedOverlay=name.compare(0,4,"mat_")==0 &&
            (EndsWith(name,"_1")||EndsWith(name,"_2"));
        if(Contains(name,"taxiway_center_marking") ||
           (hasImage && (IsOverlayImage(path)||generatedOverlay)))
            policy.layer=MaterialLayer::DECAL;
        if(policy.layer==MaterialLayer::BASE)policy.depth=GroundPriority(name);
    }

    // Preserve supplied images; procedural fallback only fills genuinely plain
    // materials. The shader should multiply this surface detail by imported Kd.
    if(!hasImage) {
        if(Contains(name,"marking")||Contains(name,"paint"))
            policy.procedural=ProceduralSurface::PAINT;
        else if(Contains(name,"metal")||Contains(name,"steel")||
                Contains(name,"aluminum")||Contains(name,"aluminium"))
            policy.procedural=ProceduralSurface::METAL;
        else if(Contains(name,"grass")||Contains(name,"scrub"))
            policy.procedural=ProceduralSurface::GRASS;
        else
            policy.procedural=ProceduralSurface::GENERAL;
    }
    return policy;
}
