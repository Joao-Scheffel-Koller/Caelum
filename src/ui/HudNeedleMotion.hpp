#ifndef CAELUM_HUD_NEEDLE_MOTION_HPP
#define CAELUM_HUD_NEEDLE_MOTION_HPP
#include "aircraft/FlightTelemetry.hpp"

// AI-assisted visual response of the needles. Does not modify flight physics.
// Only speed, throttle, altitude and heading are filtered; attitude stays real.
class HudNeedleMotion {
public:
    const FlightTelemetry& Update(const FlightTelemetry& target,double elapsedSeconds);
private:
    FlightTelemetry displayed;
};
#endif
