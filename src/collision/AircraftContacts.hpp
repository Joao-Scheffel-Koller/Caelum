#ifndef CAELUM_AIRCRAFT_CONTACTS_HPP
#define CAELUM_AIRCRAFT_CONTACTS_HPP
#include "collision/CollisionWorld.hpp"
#include <vector>

namespace collision {

// AI-assisted aircraft collision response. Position follows GameScene's pivot:
// midpoint of local X/Z bounds and minimum local Y (wheel contact level).
struct AircraftPose {
    glm::dvec3 position{0},forward{1,0,0},up{0,1,0};
};
enum class AircraftState { Grounded, Flying, Crashed };
enum class AirframeType { Airliner, LightTaildragger };
enum class CrashReason { None, HardLanding, UnsafeAttitude, Obstacle, Water,
                         AirframeContact, InvalidPose };
const char* CrashReasonText(CrashReason reason);

struct ProbeSphere {
    glm::dvec3 center{0};
    double radius=0;
    bool gear=false;
    ProbeSphere()=default;
    ProbeSphere(const glm::dvec3& c,double r,bool isGear):center(c),radius(r),gear(isGear) {}
};
struct ContactResult {
    AircraftState state=AircraftState::Flying;
    CrashReason reason=CrashReason::None;
    glm::dvec3 position{0},velocity{0},forward{1,0,0},up{0,1,0};
    bool contact=false;
    unsigned gearContacts=0;
    Hit hit;
};

class AircraftContacts {
public:
    void Configure(const glm::dvec3& modelMin,const glm::dvec3& modelMax,
                   AirframeType type=AirframeType::Airliner);
    ContactResult Resolve(const CollisionWorld& world,const AircraftPose& oldPose,
                          const AircraftPose& newPose,const glm::dvec3& velocity,double dt);
    void Reset();
    AircraftState State() const {return state;}
    CrashReason Reason() const {return reason;}
    std::vector<ProbeSphere> DebugSpheres(const AircraftPose& pose) const;
private:
    std::vector<ProbeSphere> localSpheres;
    AircraftState state=AircraftState::Flying;
    CrashReason reason=CrashReason::None;
    ContactResult frozen;
    ContactResult Crash(ContactResult result,CrashReason why,const Hit& hit);
};
} // namespace collision
#endif
