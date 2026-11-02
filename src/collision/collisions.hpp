#ifndef CAELUM_COLLISIONS_HPP
#define CAELUM_COLLISIONS_HPP

#include <glm/glm.hpp>

// AI-assisted CPU geometry. No OpenGL, renderer or camera state is required.
namespace collision {

enum class Surface { Solid, Ground, Water };

struct Aabb {
    glm::dvec3 min,max;
};
struct Triangle {
    glm::dvec3 a,b,c;
};
struct Hit {
    bool hit=false;
    double time=1;
    glm::dvec3 point{0},normal{0};
    double penetration=0;
    Surface surface=Surface::Solid;
};

Aabb Bounds(const Triangle& triangle);
Aabb SweptBounds(const glm::dvec3& from,const glm::dvec3& to,double radius);
bool Overlaps(const Aabb& a,const Aabb& b);
glm::dvec3 ClosestPoint(const glm::dvec3& point,const Triangle& triangle);

// Exact feature-based continuous test: both triangle faces, all three edges
// and vertices. Time is a fraction of [from,to], not seconds. Initial contact
// returns time zero; the normal points from the triangle toward the sphere.
// Invalid/nonfinite sweeps return an empty Hit. Geometry builders reject bad
// inputs with std::invalid_argument; Overlaps rejects malformed boxes.
Hit SweepSphere(const glm::dvec3& from,const glm::dvec3& to,double radius,
                const Triangle& triangle);

} // namespace collision
#endif
