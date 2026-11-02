// AI-assisted mathematical regressions; no GPU, window or model files needed.
#include "aircraft/BezierFlight.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(const glm::dvec3& a,const glm::dvec3& b,double tolerance=1e-6) {
    return glm::length(a-b)<tolerance;
}
template<class F> void Invalid(F function,const char* message) {
    bool rejected=false;
    try {function();}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,message);
}
void Geometry() {
    const CubicBezier c={{0,0,0},{1,2,0},{3,2,0},{4,0,0}};
    Check(Near(c.Position(0),c.p0)&&Near(c.Position(1),c.p3),"cubic interpolates endpoints");
    Check(Near(c.Position(.5),glm::dvec3(2,1.5,0)),"Bernstein midpoint");
    Check(Near(c.Tangent(0),3.0*(c.p1-c.p0))&&Near(c.Tangent(1),3.0*(c.p3-c.p2)),"endpoint derivatives");
    const double h=1e-5;
    Check(Near((c.Position(.4+h)-c.Position(.4-h))/(2*h),c.Tangent(.4)),"tangent matches numerical derivative");
    Check(Near((c.Tangent(.4+h)-c.Tangent(.4-h))/(2*h),c.SecondDerivative(.4)),"second derivative matches numerical derivative");
    const auto path=BezierPath::HorizontalCircle(glm::dvec3(4100,180,7200),1200);
    const auto& curves=path.Segments();
    Check(curves.size()==4,"four genuinely cubic segments");
    for(size_t i=0;i<curves.size();++i) {
        const auto& next=curves[(i+1)%curves.size()];
        Check(Near(curves[i].p3,next.p0),"closed circuit positional continuity");
        Check(Near(curves[i].Tangent(1),next.Tangent(0)),"closed circuit derivative continuity");
    }
    const double circumference=2*std::acos(-1.0)*1200;
    Check(std::abs(path.Length()/circumference-1)<.0004,"circle approximation and length table are accurate");
    Check(Near(path.AtDistance(0).position,glm::dvec3(5300,180,7200)),"initial traffic is ahead of Santos Dumont");
    Check(Near(path.AtDistance(0).position,path.AtDistance(path.Length()*7).position),"multiple lap wrap");
    Check(Near(path.AtDistance(-10).position,path.AtDistance(path.Length()-10).position),"negative distance wraps");
    double worst=0;
    const int samples=400;
    const double step=path.Length()/samples;
    for(int i=0;i<samples;++i) {
        const auto a=path.AtDistance(i*step),b=path.AtDistance((i+1)*step);
        worst=std::max(worst,std::abs(glm::length(b.position-a.position)/step-1));
        Check(std::abs(a.position.y-180)<1e-8,"level route keeps chosen altitude");
    }
    Check(worst<.001,"distance lookup keeps speed approximately constant");
}
void TimeAndPose() {
    BezierFlight thirty,sixty,oneFortyFour;
    for(int i=0;i<30*90;++i)thirty.Update(1.0/30);
    for(int i=0;i<60*90;++i)sixty.Update(1.0/60);
    for(int i=0;i<144*90;++i)oneFortyFour.Update(1.0/144);
    Check(std::abs(thirty.Distance()-5400)<1e-7,"Euler integrates 60m/s for90s");
    Check(Near(thirty.Pose().position,sixty.Pose().position,.001)&&
          Near(thirty.Pose().position,oneFortyFour.Pose().position,.001),"same pose at30/60/144fps");
    BezierFlight jump;
    jump.Update(90);
    Check(Near(jump.Pose().position,sixty.Pose().position,.001),"long dt advances full elapsed time");
    BezierFlight oneLap;
    const auto initial=oneLap.Pose();
    oneLap.Update(oneLap.LapLength()/oneLap.Speed());
    Check(Near(initial.position,oneLap.Pose().position,.001)&&Near(initial.up,oneLap.Pose().up),"lap wrap has no position or orientation jump");
    for(int junction=1;junction<=4;++junction) {
        BezierFlight beforeJoin,afterJoin;
        const double boundary=oneLap.LapLength()*junction/4;
        beforeJoin.Update((boundary-.01)/oneLap.Speed());
        afterJoin.Update((boundary+.01)/oneLap.Speed());
        const auto a=beforeJoin.Pose(),b=afterJoin.Pose();
        Check(std::abs(glm::length(a.position-b.position)-.02)<.001,
              "position passes smoothly through each join, including lap closure");
        Check(Near(a.forward,b.forward,.0001),"heading is continuous across each join");
        Check(Near(a.up,b.up,.0001),"bank is continuous across each join");
    }
    BezierFlight repeated,singleStep;
    for(int i=0;i<1200;++i)repeated.Update(.4);
    singleStep.Update(480);
    Check(Near(repeated.Pose().position,singleStep.Pose().position,.001),
          "many frame updates and one elapsed interval agree after multiple laps");
    Check(Near(repeated.Pose().forward,singleStep.Pose().forward),
          "multiple laps preserve the expected heading");
    const auto before=jump.Pose();
    jump.Update(-1);jump.Update(0);jump.Update(std::numeric_limits<double>::quiet_NaN());
    Check(Near(before.position,jump.Pose().position),"invalid time cannot corrupt animation");
    for(int i=0;i<500;++i) {
        jump.Update(.3);
        const auto p=jump.Pose();
        Check(std::isfinite(p.position.x)&&std::isfinite(p.forward.y)&&std::isfinite(p.up.z),"finite flight pose");
        Check(std::abs(glm::length(p.forward)-1)<1e-6&&std::abs(glm::length(p.up)-1)<1e-6&&
              std::abs(glm::length(p.right)-1)<1e-6,"unit basis");
        Check(std::abs(glm::dot(p.forward,p.up))<1e-6&&Near(glm::cross(p.forward,p.up),p.right),"right handed orthogonal basis");
        Check(glm::dot(p.up,glm::vec3(0,1,0))>=std::cos(25.001*std::acos(-1.0)/180),"bank limited to25degrees");
        const auto matrix=p.Orientation();
        Check(Near(glm::vec3(matrix*glm::vec4(1,0,0,0)),p.forward),"model nose+X follows tangent");
    }
    BezierFlight paused(BezierPath::HorizontalCircle(glm::dvec3(0),10),0);
    const auto still=paused.Pose();paused.Update(100);
    Check(Near(still.position,paused.Pose().position)&&Near(still.up,glm::vec3(0,1,0)),"zero speed remains stationary and level");
}
void InvalidPaths() {
    Invalid([]{BezierPath p({});},"empty path rejected");
    Invalid([]{BezierPath::HorizontalCircle(glm::dvec3(0),0);},"zero radius rejected");
    Invalid([]{BezierFlight f(BezierPath::HorizontalCircle(glm::dvec3(0),1),-1);},"negative speed rejected");
    Invalid([]{BezierPath p({{{0,0,0},{0,0,0},{0,0,0},{0,0,0}}});},"degenerate curve rejected");
    auto curves=BezierPath::HorizontalCircle(glm::dvec3(0),1).Segments();
    curves[0].p3.x+=1;
    Invalid([&]{BezierPath p(curves);},"disconnected loop rejected");
    curves=BezierPath::HorizontalCircle(glm::dvec3(0),1).Segments();
    curves[0].p1=curves[0].p0-(curves[0].p1-curves[0].p0);
    Invalid([&]{BezierPath p(curves);},"direction discontinuity rejected");
    curves[0].p1.x=std::numeric_limits<double>::quiet_NaN();
    Invalid([&]{BezierPath p(curves);},"nonfinite controls rejected");
}
}
int main() {
    Geometry();TimeAndPose();InvalidPaths();
    std::printf("PASS: %d Bezier geometry, Euler, speed and orientation checks\n",checks);
}
