#ifndef CAELUM_BEZIER_FLIGHT_HPP
#define CAELUM_BEZIER_FLIGHT_HPP
#include "animation/BezierPath.hpp"

struct FlightPose {
    glm::vec3 position,forward,up,right;
    glm::mat4 Orientation() const;
};
// AI-assisted kinematic traffic: owns CPU path/state, never the shared GPU model.
class BezierFlight {
public:
    BezierFlight();
    BezierFlight(BezierPath path,double speed);
    void Update(double dt);
    FlightPose Pose() const;
    double Distance() const {return distance;}
    double Speed() const {return speed;}
    double LapLength() const {return path.Length();}
private:
    BezierPath path;
    double speed,distance=0;
};
#endif
