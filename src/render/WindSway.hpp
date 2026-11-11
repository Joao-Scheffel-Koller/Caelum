#ifndef CAELUM_WIND_SWAY_HPP
#define CAELUM_WIND_SWAY_HPP
// AI-assisted: procedural wind on the map's tree billboards (docs/wind.md).
// The displacement runs in src/render/shaders/shader_vertex.glsl; these
// parameters are sent to it as uniforms, and WindOffset() is the same formula
// on the CPU so the behaviour can be tested without a window.
// Technique: T. Sousa, "Vegetation Procedural Animation and Shading in Crysis",
// GPU Gems 3, ch. 16 (2007).
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct WindParameters {
    glm::vec2 direction{0.8f,0.6f};  // Horizontal wind direction (XZ, unit).
    float amplitude=0.30f;           // m: sway at the top of a tree (v=1).
    float angularFrequency=2.1f;     // rad/s: one main sway every ~3 s.
    float wavenumber=0.05f;          // rad/m: gusts ~125 m long cross the city.
    float flutter=0.04f;             // m: fast leaf rustle on the crown.
    float flutterFrequency=9.0f;     // rad/s.
};

// Offset added to a tree vertex at world position p. height01 is the texture
// coordinate v of the billboard (0 = base, 1 = top); time in seconds.
glm::vec3 WindOffset(const WindParameters& wind,const glm::vec3& p,float height01,float time);

// Time-based wind clock: advanced with the frame's elapsed time (dt), so the
// motion has the same speed on any computer and stops while paused.
class WindClock {
public:
    void Update(double dt) {if(dt>0)time+=dt;}
    float Time() const {return static_cast<float>(time);}
private:
    double time=0;
};
#endif
