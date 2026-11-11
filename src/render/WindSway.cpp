#include "render/WindSway.hpp"
#include <algorithm>
#include <cmath>

// Keep identical to the wind block in src/render/shaders/shader_vertex.glsl.
glm::vec3 WindOffset(const WindParameters& wind,const glm::vec3& p,float height01,float time) {
    const float v=std::max(0.0f,std::min(1.0f,height01));
    // Bending: the base stays rooted, the top moves the most (v^2).
    const float bend=v*v;
    // Phase from the position along the wind: neighbouring trees move almost
    // together and the gust travels across the city with the wind.
    const float phase=-wind.wavenumber*(p.x*wind.direction.x+p.z*wind.direction.y);
    // Two waves with a non-integer ratio: irregular, not a visible rhythm.
    const float sway=0.7f*std::sin(wind.angularFrequency*time+phase)+
                     0.3f*std::sin(2.3f*wind.angularFrequency*time+1.7f*phase);
    // Leaf rustle: small and fast, only on the crown, own phase per vertex.
    const float crown=std::max(0.0f,std::min(1.0f,(v-0.4f)/0.6f));
    const float rustle=std::sin(wind.flutterFrequency*time+0.37f*p.x+0.71f*p.z);
    const float along=wind.amplitude*bend*sway+wind.flutter*crown*rustle;
    return glm::vec3(wind.direction.x*along,0.0f,wind.direction.y*along);
}
