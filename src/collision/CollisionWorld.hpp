#ifndef CAELUM_COLLISION_WORLD_HPP
#define CAELUM_COLLISION_WORLD_HPP
#include "collision/collisions.hpp"
#include <glm/vec3.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace collision {

// AI-assisted CPU collision data. Geometry stays compact in map float units;
// only candidate triangles are promoted to doubles for narrow-phase queries.
struct WorldTriangle {
    glm::vec3 a,b,c;
    Surface surface=Surface::Solid;
    WorldTriangle()=default;
    WorldTriangle(const glm::vec3& first,const glm::vec3& second,const glm::vec3& third,
                  Surface kind=Surface::Solid):a(first),b(second),c(third),surface(kind) {}
};

enum class GeometryKind { Ignore, Ground, Water, Solid, Tree };

// Pass the existing renderer's decal classification; alpha cutout trees need
// physical proxies instead of collisions against the complete transparent quad.
GeometryKind ClassifyCollisionMaterial(const std::string& name,const std::string& diffusePath,
                                      bool decal);

// Append a closed octagonal trunk and a smaller conical crown, within the
// billboard dimensions. centerBase is the bottom-center in world coordinates.
// Invalid/nonpositive dimensions are ignored. All proxy faces are Solid.
void AppendTreeProxy(std::vector<WorldTriangle>& output,const glm::dvec3& centerBase,
                     double height,double width);

class CollisionWorld {
public:
    // Empty/degenerate chunks are ignored. Invalid coordinates are rejected.
    // Each chunk builds an independent, balanced BVH from its actual vertices;
    // no assumption is made about its filename or its nominal grid cell.
    void AddChunk(std::string name,std::vector<WorldTriangle> triangles);
    // Supported wheels may ignore upward ground contacts while still finding
    // later obstacles/water and unsafe ground edges along the same movement.
    Hit SweepSphere(const glm::dvec3& from,const glm::dvec3& to,double radius,
                    std::size_t* tested=nullptr,bool ignoreSupport=false) const;
    // Optional QA sample from retained physical geometry. Absence leaves the
    // output untouched; this is not a per-frame spatial query.
    bool SampleSurface(Surface kind,Triangle& triangle) const;
    std::size_t TriangleCount() const {return triangleCount;}
    std::size_t ChunkCount() const {return chunks.size();}
private:
    struct Node {
        glm::vec3 min,max;
        std::uint32_t first=0,count=0,right=0; // count==0 means internal node.
    };
    struct Chunk {
        std::string name;
        std::vector<WorldTriangle> triangles;
        std::vector<Node> nodes;
    };
    std::vector<Chunk> chunks;
    std::size_t triangleCount=0;
    static std::uint32_t Build(Chunk& chunk,std::uint32_t begin,std::uint32_t end);
};

} // namespace collision
#endif
