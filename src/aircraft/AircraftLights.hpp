#ifndef CAELUM_AIRCRAFT_LIGHTS_HPP
#define CAELUM_AIRCRAFT_LIGHTS_HPP

#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace aircraftlights {

enum class Pattern { Steady, Beacon, Strobe };

// Positions and radii use the mesh's local units: +X nose, +Y up, +Z right.
// phaseOffset is measured in seconds; a positive offset advances the pattern.
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float haloRadius;
    Pattern pattern;
    double phaseOffset;
};

// Surveyed anchors for catalog meshes. Unknown IDs throw std::invalid_argument.
const std::vector<Light>& Profile(const std::string& aircraftId);

// Visual timing only, not an aviation certification model. Result is in [0,1].
float Intensity(Pattern pattern, double time, double phaseOffset=0);

class Clock {
public:
    // Ignore negative/nonfinite steps and additions that would overflow.
    // The caller pauses the clock by omitting Update while simulation is paused.
    void Update(double dt);
    void Reset();
    double Time() const;
private:
    double time_=0;
};

} // namespace aircraftlights

#endif
