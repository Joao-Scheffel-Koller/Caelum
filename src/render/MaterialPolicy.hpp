#ifndef CAELUM_MATERIAL_POLICY_HPP
#define CAELUM_MATERIAL_POLICY_HPP
#include <string>

// AI-assisted material classification. No OpenGL or asset loading dependencies.
enum class MaterialLayer { BASE, DECAL };
enum class ProceduralSurface { NONE=0, GENERAL=1, GRASS=2, METAL=3, PAINT=4 };

// AI-assisted: OSM2World exports ground cover, pavements and lane/runway
// surfaces as overlapping polygons on the same plane (Y=0). Opaque layers that
// share a plane are separated by a depth priority instead of moving geometry
// (which also feeds the collision world). Higher = pushed farther back.
enum class DepthPriority { SURFACE=0, PAVEMENT=1, NATURAL_GROUND=2 };

struct MaterialPolicy {
    MaterialLayer layer=MaterialLayer::BASE;
    ProceduralSurface procedural=ProceduralSurface::NONE;
    DepthPriority depth=DepthPriority::SURFACE;
    bool sway=false;   // AI-assisted: map tree billboards move with the wind (docs/wind.md).
};

MaterialPolicy ClassifyMaterial(const std::string& name,const std::string& diffusePath,
                               bool hasImage,bool isMap);
#endif
