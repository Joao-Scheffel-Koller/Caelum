#ifndef CAELUM_MATERIAL_POLICY_HPP
#define CAELUM_MATERIAL_POLICY_HPP
#include <string>

// AI-assisted material classification. No OpenGL or asset loading dependencies.
enum class MaterialLayer { BASE, DECAL };
enum class ProceduralSurface { NONE=0, GENERAL=1, GRASS=2, METAL=3, PAINT=4 };

struct MaterialPolicy {
    MaterialLayer layer=MaterialLayer::BASE;
    ProceduralSurface procedural=ProceduralSurface::NONE;
};

MaterialPolicy ClassifyMaterial(const std::string& name,const std::string& diffusePath,
                               bool hasImage,bool isMap);
#endif
