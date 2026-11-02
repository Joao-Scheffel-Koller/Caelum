#include "animation/BezierPath.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
bool Finite(const glm::dvec3& p) {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
void CheckParameter(double u) {
    if(!std::isfinite(u)||u<0||u>1)throw std::invalid_argument("Bezier parameter must be in [0,1]");
}
}
glm::dvec3 CubicBezier::Position(double u) const {
    CheckParameter(u);
    const double v=1-u;
    return v*v*v*p0+3*v*v*u*p1+3*v*u*u*p2+u*u*u*p3;
}
glm::dvec3 CubicBezier::Tangent(double u) const {
    CheckParameter(u);
    const double v=1-u;
    return 3.0*(v*v*(p1-p0)+2*v*u*(p2-p1)+u*u*(p3-p2));
}
glm::dvec3 CubicBezier::SecondDerivative(double u) const {
    CheckParameter(u);
    return 6.0*((1-u)*(p2-2.0*p1+p0)+u*(p3-2.0*p2+p1));
}
BezierPath::BezierPath(std::vector<CubicBezier> curves,unsigned samplesPerSegment)
    :segments(std::move(curves)) {
    if(segments.empty()||samplesPerSegment<2)throw std::invalid_argument("Bezier path needs curves and at least two samples");
    for(size_t i=0;i<segments.size();++i) {
        const auto& a=segments[i];const auto& b=segments[(i+1)%segments.size()];
        if(!Finite(a.p0)||!Finite(a.p1)||!Finite(a.p2)||!Finite(a.p3))
            throw std::invalid_argument("Nonfinite Bezier control point");
        const auto end=a.Tangent(1),start=b.Tangent(0);
        if(glm::length(a.p3-b.p0)>1e-7||glm::length(end)<1e-9||glm::length(start)<1e-9||
           glm::dot(glm::normalize(end),glm::normalize(start))<1.0-1e-7)
            throw std::invalid_argument("Closed Bezier path must have continuous positions and directions");
    }
    knots.push_back({0,0});
    for(size_t i=0;i<segments.size();++i) {
        glm::dvec3 previous=segments[i].p0;
        for(unsigned j=1;j<=samplesPerSegment;++j) {
            const double u=double(j)/samplesPerSegment;
            const auto position=segments[i].Position(u),tangent=segments[i].Tangent(u);
            const double step=glm::length(position-previous);
            if(!std::isfinite(step)||step<1e-12||glm::length(tangent)<1e-9)
                throw std::invalid_argument("Degenerate Bezier path");
            length+=step;
            knots.push_back({length,double(i)+u});
            previous=position;
        }
    }
}
BezierPath BezierPath::HorizontalCircle(const glm::dvec3& center,double radius) {
    if(!Finite(center)||!std::isfinite(radius)||radius<=0)throw std::invalid_argument("Invalid circle");
    // Four cubic quarter-arcs. Equal endpoint handles give matching derivatives.
    const double k=4.0*std::tan(std::acos(-1.0)/8.0)/3.0;
    const glm::dvec3 x(radius,0,0),z(0,0,radius);
    return BezierPath({
        {center+x,center+x+k*z,center+k*x+z,center+z},
        {center+z,center-k*x+z,center-x+k*z,center-x},
        {center-x,center-x-k*z,center-k*x-z,center-z},
        {center-z,center+k*x-z,center+x-k*z,center+x}
    });
}
BezierSample BezierPath::AtDistance(double distance) const {
    if(!std::isfinite(distance))throw std::invalid_argument("Nonfinite Bezier distance");
    distance=std::fmod(distance,length);
    if(distance<0)distance+=length;
    const auto end=std::lower_bound(knots.begin(),knots.end(),distance,
        [](const Knot& knot,double value){return knot.distance<value;});
    double parameter=0;
    if(end!=knots.begin()) {
        const auto& before=*(end-1);
        parameter=before.parameter+(end->parameter-before.parameter)*
            ((distance-before.distance)/(end->distance-before.distance));
    }
    const size_t index=std::min(size_t(parameter),segments.size()-1);
    const double u=std::max(0.0,std::min(1.0,parameter-index));
    const auto& curve=segments[index];
    const auto tangent=curve.Tangent(u);
    if(glm::length(tangent)<1e-9)throw std::runtime_error("Undefined Bezier direction");
    return {curve.Position(u),tangent,curve.SecondDerivative(u)};
}
