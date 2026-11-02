#ifndef CAELUM_FLIGHT_MISSION_HPP
#define CAELUM_FLIGHT_MISSION_HPP
#include "mission/Airports.hpp"
#include "collision/AircraftContacts.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mission {
enum class Stage { Departure, EnRoute, Approach, Landing, Completed, Failed };
struct MissionSample {
    glm::dvec3 position{0},velocity{0},forward{1,0,0};
    collision::AircraftState aircraftState=collision::AircraftState::Flying;
    unsigned gearContacts=0;
    std::vector<glm::dvec3> gearCenters;
    // World-space centers and matching sphere radii from AircraftContacts.
    std::vector<double> gearRadii;
};
struct MissionSnapshot {
    Stage stage=Stage::Departure;
    std::string instruction,targetName;
    glm::dvec3 target{0};
    double distanceMeters=0,bearingDegrees=0,flightSeconds=0,landingSeconds=0;
    bool originArmed=false,departed=false;
};

// AI-assisted mission state machine. Reads only the corrected player state;
// no camera, renderer, physics mutation or input calls belong to this module.
class FlightMission {
public:
    FlightMission(const Airport& origin=SantosDumont(),const Airport& destination=Galeao());
    void Reset();
    const MissionSnapshot& Update(const MissionSample& sample,double elapsedSeconds);
    const MissionSnapshot& Snapshot() const {return snapshot;}
private:
    Airport origin,destination;
    MissionSnapshot snapshot;
    bool previousValid=false,previousFlying=false,liftoffPending=false;
    bool previousLandingQualified=false,arrivalArmed=false;
    std::size_t originRunway=0,destinationRunway=0,previousLandingRunway=0,arrivalRunway=0;
    double airborneSeconds=0,departureHeight=0;
    void Guidance(const MissionSample& sample);
};
} // namespace mission
#endif
