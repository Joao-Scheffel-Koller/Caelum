#ifndef CAELUM_FLIGHT_TELEMETRY_HPP
#define CAELUM_FLIGHT_TELEMETRY_HPP
#include <glm/glm.hpp>

// AI-assisted, read-only flight measurements. No graphics or physics updates.
// Invalid channels retain a finite zero value; callers must honor their flags.
struct FlightTelemetry {
    double altitudeMeters=0;
    double speedKmh=0;
    double throttlePercent=0;
    double headingDegrees=0;
    double pitchDegrees=0;
    double rollDegrees=0;
    bool altitudeValid=false;
    bool speedValid=false;
    bool throttleValid=false;
    bool headingValid=false;
    bool attitudeValid=false;
};

// World convention: +Y up, north -Z, east +X. Heading is [0,360).
// Positive pitch raises the nose; positive roll lowers the right wing.
// Altitude is relative to map Y=0, not a terrain clearance or surveyed elevation.
// Speed is world velocity magnitude; throttle is a command, not measured power.
// At a vertical heading, heading/roll are undefined and their flags are false.
FlightTelemetry CalculateFlightTelemetry(const glm::vec3& position,
    const glm::vec3& velocity,const glm::vec3& forward,const glm::vec3& up,
    double throttle);
#endif
