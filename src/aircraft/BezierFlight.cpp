#include "aircraft/BezierFlight.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

glm::mat4 FlightPose::Orientation() const {
    // Model convention: nose +X, up +Y, right +Z. Construct our own basis.
    glm::mat4 result(1);
    result[0]=glm::vec4(forward,0);
    result[1]=glm::vec4(up,0);
    result[2]=glm::vec4(right,0);
    return result;
}
BezierFlight::BezierFlight()
    :BezierFlight(BezierPath::HorizontalCircle(glm::dvec3(4100,180,7200),1200),60) {}
BezierFlight::BezierFlight(BezierPath route,double velocity)
    :path(std::move(route)),speed(velocity) {
    if(!std::isfinite(speed)||speed<0)throw std::invalid_argument("Invalid traffic speed");
}
void BezierFlight::Update(double dt) {
    if(!std::isfinite(dt)||dt<=0)return;
    // FONTE: Prof. Eduardo Gastal, Aula_24_Animacao.pdf, slides 10 e 18.
    // Explicit Euler applied to arc distance: s(n+1)=s(n)+speed*dt.
    // Constant speed makes this step exact; arc-length inversion is approximate.
    // Reduce dt by the lap period first to support long gaps without overflow.
    if(speed==0)return;
    const double stepTime=std::fmod(dt,path.Length()/speed);
    distance+=speed*stepTime; // Euler: distance += velocity * time step.
    distance=std::fmod(distance,path.Length());
}
FlightPose BezierFlight::Pose() const {
    const auto sample=path.AtDistance(distance);
    const auto forward=glm::normalize(sample.tangent);
    glm::dvec3 right=glm::cross(forward,glm::dvec3(0,1,0));
    if(glm::length(right)<1e-9)right=glm::cross(forward,glm::dvec3(0,0,1));
    right=glm::normalize(right);
    const auto up=glm::cross(right,forward);
    const double lateralCurvature=glm::dot(sample.secondDerivative,right)/glm::dot(sample.tangent,sample.tangent);
    const double limit=25.0*std::acos(-1.0)/180.0;
    const double bank=std::max(-limit,std::min(limit,std::atan(speed*speed*lateralCurvature/9.81)));
    const auto bankedUp=glm::normalize(up*std::cos(bank)+right*std::sin(bank));
    return {glm::vec3(sample.position),glm::vec3(forward),glm::vec3(bankedUp),
            glm::vec3(glm::normalize(glm::cross(forward,bankedUp)))};
}
