#ifndef CAELUM_PROPELLER_HPP
#define CAELUM_PROPELLER_HPP
// AI-assisted: animated propeller (docs/propeller.md). The blades are split
// from the aircraft OBJ at load time and drawn with their own Model matrix,
// which turns them about the propeller shaft. Theta advances with the frame
// time at a speed set by the throttle. No OpenGL here, so it is testable.
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace propeller {
// Where the propeller is in one aircraft OBJ (nose +X, up +Y).
struct Spec {
    std::vector<std::string> blades;   // OBJ objects ("o" lines): rotate and blur.
    std::vector<std::string> spinner;  // OBJ objects: rotate, always solid.
    glm::vec3 hub{0.0f};               // Centre of the blades, on the shaft.
    // The OBJ is the source model stretched by a different factor per axis,
    // and its shaft is tilted nose-up: undo the first before turning about
    // the second, or the blades wobble.
    glm::vec3 assetScale{1.0f};        // Stretch (length X, height Y, span Z).
    float shaftTilt=0.0f;              // rad, +X towards +Y, before the stretch.
    float bladeScale=1.0f;             // Uniform scale that replaces it (round blades).
    float tipRadius=0.0f;              // m: blades as drawn, also the disc radius.
    float maxRpm=0.0f;                 // Engine speed at full throttle.
    bool Present() const {return !blades.empty();}
    Spec() {}
    Spec(const std::vector<std::string>& blades,const std::vector<std::string>& spinner,
         const glm::vec3& hub,const glm::vec3& assetScale,float shaftTilt,
         float bladeScale,float tipRadius,float maxRpm)
        :blades(blades),spinner(spinner),hub(hub),assetScale(assetScale),shaftTilt(shaftTilt),
         bladeScale(bladeScale),tipRadius(tipRadius),maxRpm(maxRpm) {}
};

// Anti-aliasing limits (docs/propeller.md, section "aliasing"). The blades
// repeat every ~120 degrees, so a frame must turn them less than 60 degrees or
// the eye sees them stop or spin backwards (wagon-wheel effect).
const float SymmetryDegrees=120.0f;   // 3 blades.
const float MinimumFps=30.0f;          // Correct at this frame rate and above.
// Fastest rotation shown with solid blades: half the symmetry per frame at
// MinimumFps -> 60 deg * 30 / 360 = 5 rev/s = 300 rpm.
const float VisibleRpm=SymmetryDegrees*.5f*MinimumFps/360.0f*60.0f;
const float BlurRpm=900.0f;            // Above this, the disc is fully formed.
const float FaintBladeOpacity=.12f;    // Blades still hinted inside the disc.
const float DiscMaxOpacity=.30f;
// Seen from the cockpit the blades never hide the runway: the asset's own
// opacity (its metadata.json made them translucent for that purpose).
const float CockpitBladeOpacity=.24f;
const float CockpitDiscOpacity=.08f;   // The disc covers the whole runway from there.
const float SpoolTime=1.0f;            // s: time constant of the engine speed.

// Engine speed and blade angle of one propeller.
class Spin {
public:
    // dt in seconds (frame time), throttle in [0,1]. Frame-rate independent.
    void Update(double dt,float throttle,float maxRpm);
    void Reset() {rpm=0;angle=0;}
    float Rpm() const {return rpm;}         // Engine speed (follows the throttle).
    float Angle() const {return angle;}     // rad, in [0, 2 pi).
private:
    float rpm=0.0f;
    float angle=0.0f;
};

// Rotation actually shown: the engine speed, capped at VisibleRpm.
float DisplayedRpm(float rpm);
// Opacity of the solid blades and of the blur disc for an engine speed.
float BladeOpacity(float rpm,bool cockpit=false);
float DiscOpacity(float rpm,bool cockpit=false);

// Tilt of the blades' plane normal in the (stretched) OBJ, rad.
float PlaneTilt(const Spec& spec);
// All three map aircraft OBJ coordinates to aircraft OBJ coordinates.
// Positive angles turn clockwise seen from the cockpit (up -> right).
// Blades: T(hub) S(bladeScale) R_z(plane) R_x(angle) R_z(-shaft) S(1/asset) T(-hub)
// i.e. undo the stretch, align the shaft with X, turn, tilt back, uniform scale.
glm::mat4 BladeMatrix(const Spec& spec,float angle);
// Spinner: the same turn, but stretched back like the asset, so it keeps
// fitting the (equally stretched) engine cowling.
glm::mat4 SpinnerMatrix(const Spec& spec,float angle);
// Blur disc (built at the origin in the YZ plane) -> the blades' plane.
glm::mat4 DiscMatrix(const Spec& spec);
} // namespace propeller
#endif
