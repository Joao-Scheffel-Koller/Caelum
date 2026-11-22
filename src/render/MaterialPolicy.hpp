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
// One level per pair of materials measured overlapping in the export, so no
// two overlapping ground materials share a level (docs/zfighting.md).
enum class DepthPriority {
    SURFACE=0,       // Markings, runway/pitch bases, railways, roofs.
    ASPHALT=1,       // Roads cross over plazas and paved areas.
    SETT=2,          // Sett, gravel, grass paver.
    CONCRETE=3,      // Concrete, paving stone.
    COBBLESTONE=4,   // Unhewn cobblestone, pebblestone.
    GROUND_PATCH=5,  // Sand, scree, water, wetland: patches inside parks.
    BARE_GROUND=6,   // Earth, scrub: fields and footpaths over grass.
    GRASS=7          // Grass, meadow: the background of every park.
};

struct MaterialPolicy {
    MaterialLayer layer=MaterialLayer::BASE;
    ProceduralSurface procedural=ProceduralSurface::NONE;
    DepthPriority depth=DepthPriority::SURFACE;
    bool sway=false;   // AI-assisted: map tree billboards move with the wind (docs/wind.md).
};

MaterialPolicy ClassifyMaterial(const std::string& name,const std::string& diffusePath,
                               bool hasImage,bool isMap);
#endif
