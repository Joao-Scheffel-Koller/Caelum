#ifndef CAELUM_FLIGHT_CONTROLS_HPP
#define CAELUM_FLIGHT_CONTROLS_HPP
// AI-assisted: GeoFS-style "virtual yoke" attitude control. Pure CPU math with
// no GLFW/OpenGL dependency, so it can be tested without a window.
#include <glm/vec3.hpp>

class Aircraft;

namespace flightcontrols {

// Normalized pilot commands in [-1,1].
// pitch > 0: nose up (mouse below the window center).
// roll  > 0: right wing down / turn right (mouse right of the center).
struct ControlInput {
    float pitch=0.0f;
    float roll=0.0f;
};

// Mouse shaping. Aircraft-specific rates (roll rate, steering, rotation)
// live in FlightProfile; the pitch command is a load-factor demand handled
// by FlightModel.
struct ControlTuning {
    float deadZone=0.05f;        // Fraction of the half-window ignored near the center.
    float responseExponent=1.5f; // >1 gives finer control near the center.
    float groundLevelRate=0.80f; // 1/s: nose settles on the nose/tail wheel.
    float groundAlignment=6.0f;  // 1/s: tyre grip turns the velocity with the heading.
};

// Converts a cursor position (window coordinates, origin top-left, Y down)
// into normalized yoke commands. The window center is the neutral position.
ControlInput MouseYoke(double cursorX,double cursorY,int windowWidth,int windowHeight,
                       const ControlTuning& tuning=ControlTuning());

// Body-axis rotation of v by `angle` radians around the unit vector `axis`
// (Rodrigues' rotation formula, right-handed).
glm::vec3 RotateAroundAxis(const glm::vec3& v,const glm::vec3& axis,float angle);

// Pitch/roll of the aircraft in radians, same conventions as FlightTelemetry.
float PitchAngle(const glm::vec3& forward);
float RollAngle(const glm::vec3& forward,const glm::vec3& up);

// Applies one step of attitude control to the aircraft's forward/up/right
// vectors. In flight: ailerons (roll rate) only; the turn and the pitch
// follow from the forces in FlightModel. On the ground (`grounded`, from the
// previous contact resolution): nose-wheel steering, takeoff rotation with
// speed-dependent elevator authority, and tyre grip on the velocity.
void ApplyControls(Aircraft& aircraft,const ControlInput& input,bool grounded,float dt,
                   const ControlTuning& tuning=ControlTuning());

} // namespace flightcontrols
#endif
