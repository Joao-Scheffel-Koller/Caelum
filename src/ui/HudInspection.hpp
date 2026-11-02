#ifndef CAELUM_HUD_INSPECTION_HPP
#define CAELUM_HUD_INSPECTION_HPP
#include "aircraft/FlightTelemetry.hpp"
#include "camera/CameraController.hpp"
#include <string>

struct GLFWwindow;

// AI-assisted developer-only HUD fixtures and GPU screenshots.
// Prepare before rendering; Telemetry substitutes fixtures only in this mode;
// Capture must run after HUD rendering and before swapping the back buffer.
class HudInspection {
public:
    explicit HudInspection(const std::string& prefix):prefix(prefix) {}
    void Prepare(GLFWwindow* window,CameraController& camera,const CameraFrame& aircraft) const;
    FlightTelemetry Telemetry(const FlightTelemetry& real) const;
    bool Capture(GLFWwindow* window);
private:
    std::string prefix;
    int frame=0;
    mutable FlightTelemetry displayed;
};
#endif
