#include "aircraft/AircraftLights.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aircraftlights {
namespace {
const double Period=1.2;

double SmoothEdge(double t) {
    t=std::max(0.0,std::min(1.0,t));
    return t*t*(3-2*t);
}

float Pulse(double time,double start,double duration,double edge) {
    const double elapsed=time-start;
    if(elapsed<=0||elapsed>=duration)return 0;
    return static_cast<float>(SmoothEdge(elapsed/edge)*SmoothEdge((duration-elapsed)/edge));
}
} // namespace

const std::vector<Light>& Profile(const std::string& aircraftId) {
    // data/aircraft/aircraft.obj survey: main-wing Cube tips are near
    // (-10.93,.69,+/-17.18), rather than at the fuselage's minimum X.
    // Strobes use the aft part of that same tip. Both clear the mesh laterally.
    // Cube.013 and Cube.074 supply the top and belly beacon attachment points;
    // the white aft lamp clears the Cylinder tailcone at X=-27.425.
    static const std::vector<Light> jet={
        {{-10.93f,.72f,-17.24f},{1.0f,.025f,.015f},.055f,.14f,Pattern::Steady,0},
        {{-10.93f,.72f, 17.24f},{.025f,1.0f,.08f},.055f,.14f,Pattern::Steady,0},
        {{-27.50f,1.10f,0},{1.0f,1.0f,1.0f},.05f,.13f,Pattern::Steady,0},
        {{-7.31f,2.30f,0},{1.0f,.025f,.015f},.07f,.18f,Pattern::Beacon,0},
        {{-8.03f,-2.40f,0},{1.0f,.025f,.015f},.07f,.18f,Pattern::Beacon,0},
        {{-11.57f,.73f,-17.22f},{1.0f,1.0f,1.0f},.065f,.17f,Pattern::Strobe,0},
        {{-11.57f,.73f, 17.22f},{1.0f,1.0f,1.0f},.065f,.17f,Pattern::Strobe,0}
    };
    // data/light_aircraft/aircraft.obj: Plane.002 leading/trailing tip vertices
    // are (2.65052,2.87249,+/-5.5) and (.82821,2.21240,+/-5.5).
    // The Tail fin carries a beacon above (-3.81001,3,0), with a white lamp
    // just aft of its trailing vertex (-4.5,2.81906,0).
    static const std::vector<Light> light={
        {{2.65f,2.89f,-5.56f},{1.0f,.025f,.015f},.035f,.09f,Pattern::Steady,0},
        {{2.65f,2.89f, 5.56f},{.025f,1.0f,.08f},.035f,.09f,Pattern::Steady,0},
        {{-4.56f,2.82f,0},{1.0f,1.0f,1.0f},.035f,.09f,Pattern::Steady,0},
        {{-3.81f,3.07f,0},{1.0f,.025f,.015f},.045f,.12f,Pattern::Beacon,0},
        {{.83f,2.25f,-5.56f},{1.0f,1.0f,1.0f},.04f,.105f,Pattern::Strobe,0},
        {{.83f,2.25f, 5.56f},{1.0f,1.0f,1.0f},.04f,.105f,Pattern::Strobe,0}
    };
    if(aircraftId=="a320")return jet;
    if(aircraftId=="light")return light;
    throw std::invalid_argument("Unknown aircraft light profile: "+aircraftId);
}

float Intensity(Pattern pattern,double time,double phaseOffset) {
    if(pattern==Pattern::Steady)return 1;
    if(!std::isfinite(time)||!std::isfinite(phaseOffset))return 0;
    // Reduce before adding so large but finite values cannot overflow.
    double phase=std::fmod(std::fmod(time,Period)+std::fmod(phaseOffset,Period),Period);
    if(phase<0)phase+=Period;
    if(pattern==Pattern::Beacon)return Pulse(phase,0,.16,.02);
    if(pattern==Pattern::Strobe)
        return std::max(Pulse(phase,0,.065,.008),Pulse(phase,.16,.065,.008));
    return 0;
}

void Clock::Update(double dt) {
    if(!std::isfinite(dt)||dt<0)return;
    const double next=time_+dt;
    if(std::isfinite(next))time_=next;
}

void Clock::Reset() { time_=0; }
double Clock::Time() const { return time_; }

} // namespace aircraftlights
