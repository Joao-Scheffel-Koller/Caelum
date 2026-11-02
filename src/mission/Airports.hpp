#ifndef CAELUM_MISSION_AIRPORTS_HPP
#define CAELUM_MISSION_AIRPORTS_HPP
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace mission {

// AI-assisted geometry of the explicit runway rectangles. World coordinates
// use meters, +Y up and north=-Z. Surface/contact height is checked separately.
struct Runway {
    std::string id,name;
    glm::dvec3 start,end;
    double width;
};
struct Airport {
    std::string id,name;
    std::vector<Runway> runways;
};

const Airport& SantosDumont();
const Airport& Galeao();

// Positive inset reduces both ends and both sides. Invalid/empty rectangles,
// nonfinite positions and negative insets fail closed. Y is not containment.
bool Contains(const Runway& runway,const glm::dvec3& point,double inset=0);
// Horizontal Euclidean distance to the rectangular area, zero inside. Invalid
// geometry/positions return infinity. Runway endpoints have square end caps.
double DistanceToRunway(const Runway& runway,const glm::dvec3& point);
// Clockwise degrees [0,360), north=-Z. Coincident horizontal positions return
// zero; nonfinite positions throw std::invalid_argument.
double BearingToTarget(const glm::dvec3& from,const glm::dvec3& to);

} // namespace mission
#endif
