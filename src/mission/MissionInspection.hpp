#ifndef CAELUM_MISSION_INSPECTION_HPP
#define CAELUM_MISSION_INSPECTION_HPP
#include "mission/FlightMission.hpp"
#include <string>

struct GLFWwindow;

// AI-assisted developer-only UI fixtures. They do not change FlightMission,
// Aircraft, collisions or normal gameplay. Readback runs before buffer swap.
class MissionInspection {
public:
    explicit MissionInspection(const std::string& prefix):prefix(prefix) {}
    void Prepare(GLFWwindow* window) const;
    mission::MissionSnapshot Preview(const mission::MissionSnapshot& live) const;
    bool Capture(GLFWwindow* window);
private:
    std::string prefix;
    int frame=0;
    mutable double viewStarted=0;
    mutable mission::MissionSnapshot displayed;
};
#endif
