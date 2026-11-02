#ifndef CAELUM_MISSION_OVERLAY_HPP
#define CAELUM_MISSION_OVERLAY_HPP

#include "mission/FlightMission.hpp"

struct GLFWwindow;

// AI-assisted screen-space presentation. Reads the mission snapshot only;
// visibility and the actual player's mission updates belong to the caller.
class MissionOverlay {
public:
    void Draw(GLFWwindow* window,const mission::MissionSnapshot& snapshot) const;
};

#endif
