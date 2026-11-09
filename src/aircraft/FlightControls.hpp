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

// Gameplay tuning, not aircraft data. All rates are in radians per second.
struct ControlTuning {
    float deadZone=0.05f;        // Fraction of the half-window ignored near the center.
    float responseExponent=1.5f; // >1 gives finer control near the center.
    float maxPitchRate=0.50f;    // Elevator: rotation about the aircraft right axis.
    float maxRollRate=1.00f;     // Ailerons: rotation about the aircraft forward axis.
    float directYawRate=0.10f;   // Small rudder-like yaw from the lateral command.
    float turnRate=0.60f;        // Coordinated turn: yaw rate = turnRate*sin(bank).
    float rollStability=0.50f;   // Gentle return to wings level (upright only) with a centered mouse.
    float groundSteerRate=0.50f; // Nose-wheel steering at/above groundSteerSpeed.
    float groundSteerSpeed=5.0f; // m/s: steering grows linearly up to this speed.
    float groundLevelRate=0.80f; // Pitch/roll return to the ground attitude.
    float airAlignment=1.5f;     // 1/s: velocity direction follows the nose in flight.
    float groundAlignment=6.0f;  // 1/s: tire grip on the ground.
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
// vectors and aligns its velocity with the nose. `grounded` comes from the
// previous contact resolution: on the ground the lateral command steers the
// nose wheel and pitch/roll are held at the ground attitude.
void ApplyControls(Aircraft& aircraft,const ControlInput& input,bool grounded,float dt,
                   const ControlTuning& tuning=ControlTuning());

} // namespace flightcontrols
#endif
