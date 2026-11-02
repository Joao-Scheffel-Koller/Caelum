#ifndef CAELUM_BEZIER_PATH_HPP
#define CAELUM_BEZIER_PATH_HPP
#include <glm/glm.hpp>
#include <vector>

// AI-assisted CPU-only cubic geometry; no graphics or flight physics dependency.
struct CubicBezier {
    glm::dvec3 p0,p1,p2,p3;
    glm::dvec3 Position(double u) const;
    glm::dvec3 Tangent(double u) const;
    glm::dvec3 SecondDerivative(double u) const;
};
struct BezierSample {
    glm::dvec3 position,tangent,secondDerivative;
};
class BezierPath {
public:
    explicit BezierPath(std::vector<CubicBezier> segments,unsigned samplesPerSegment=128);
    static BezierPath HorizontalCircle(const glm::dvec3& center,double radius);
    BezierSample AtDistance(double distance) const;
    double Length() const {return length;}
    const std::vector<CubicBezier>& Segments() const {return segments;}
private:
    struct Knot {double distance,parameter;};
    std::vector<CubicBezier> segments;
    std::vector<Knot> knots;
    double length=0;
};
#endif
