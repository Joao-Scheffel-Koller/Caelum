#include "aircraft/FlightTelemetry.hpp"
#include <algorithm>
#include <cmath>

namespace {
const double Degrees=180.0/std::acos(-1.0);
const double AngularEpsilon=1e-8;

bool Finite(const glm::dvec3& value) {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}
bool Unit(const glm::vec3& input,glm::dvec3& output) {
    const glm::dvec3 value(input);
    if(!Finite(value))return false;
    const double length=std::sqrt(glm::dot(value,value));
    if(!std::isfinite(length)||length==0)return false;
    output=value/length;
    return true;
}
}

FlightTelemetry CalculateFlightTelemetry(const glm::vec3& position,
    const glm::vec3& velocity,const glm::vec3& forward,const glm::vec3& up,
    double throttle) {
    FlightTelemetry result;
    if(std::isfinite(position.y)) {
        result.altitudeMeters=position.y;
        result.altitudeValid=true;
    }
    const glm::dvec3 movement(velocity);
    if(Finite(movement)) {
        result.speedKmh=std::sqrt(glm::dot(movement,movement))*3.6;
        result.speedValid=true;
    }
    if(std::isfinite(throttle)) {
        result.throttlePercent=std::max(0.0,std::min(1.0,throttle))*100.0;
        result.throttleValid=true;
    }

    glm::dvec3 front,providedUp;
    if(!Unit(forward,front))return result;
    const double horizontal=std::hypot(front.x,front.z);
    result.pitchDegrees=std::atan2(front.y,horizontal)*Degrees;
    if(horizontal<=AngularEpsilon)return result;

    double heading=std::atan2(front.x,-front.z)*Degrees;
    if(heading<0)heading+=360.0;
    result.headingDegrees=heading>=360.0||heading==0?0.0:heading;
    result.headingValid=true;
    if(!Unit(up,providedUp))return result;

    // Same Gram-Schmidt basis as GameScene: right=cross(forward,up), then
    // correctedUp=cross(right,forward). Only vector operations are used.
    glm::dvec3 right=glm::cross(front,providedUp);
    const double rightLength=glm::length(right);
    if(rightLength<=AngularEpsilon)return result;
    right/=rightLength;
    const glm::dvec3 correctedUp=glm::cross(right,front);
    const glm::dvec3 levelRight=glm::normalize(glm::cross(front,glm::dvec3(0,1,0)));
    const glm::dvec3 levelUp=glm::cross(levelRight,front);
    result.rollDegrees=std::atan2(glm::dot(correctedUp,levelRight),
                                glm::dot(correctedUp,levelUp))*Degrees;
    result.attitudeValid=true;
    return result;
}
