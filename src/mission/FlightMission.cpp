#include "mission/FlightMission.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace mission {
namespace {
const double RunwayInset=2.0;
const double HeightTolerance=1.0;
const double TakeoffHeight=5.0;
const double TakeoffSeconds=.5;
// A sampled liftoff can be just beyond a threshold, but must remain near the
// runway that supplied its last ground contact, not at a different airport.
const double DepartureObservationMargin=50.0;
const double LandingSeconds=2.0;
const double LandingSpeed=5.0;
const double AlignmentCosine=std::cos(25.0*std::acos(-1.0)/180.0);
const double TimeTolerance=1e-9;

bool Finite(const glm::dvec3& value) {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}
double Length(const glm::dvec3& value) {return std::hypot(std::hypot(value.x,value.y),value.z);}
bool Valid(const MissionSample& sample) {
    if(!Finite(sample.position)||!Finite(sample.velocity)||!Finite(sample.forward)||
       Length(sample.forward)<1e-9||!std::isfinite(Length(sample.forward))||sample.gearContacts>3)
        return false;
    if(sample.gearCenters.size()!=sample.gearRadii.size()||
       (!sample.gearCenters.empty()&&sample.gearCenters.size()!=3)||
       (sample.aircraftState==collision::AircraftState::Grounded&&sample.gearCenters.size()!=3))
        return false;
    for(const auto& center:sample.gearCenters)if(!Finite(center))return false;
    for(double radius:sample.gearRadii)if(!std::isfinite(radius)||radius<=0)return false;
    return true;
}
void Validate(const Airport& airport) {
    if(airport.runways.empty())throw std::invalid_argument("Mission airport has no runway");
    for(const auto& runway:airport.runways) {
        const auto center=runway.start*.5+runway.end*.5;
        const double length=std::hypot(runway.end.x-runway.start.x,runway.end.z-runway.start.z);
        if(!std::isfinite(DistanceToRunway(runway,center))||runway.width<=2*RunwayInset||
           !std::isfinite(length)||length<=2*RunwayInset)
            throw std::invalid_argument("Mission runway has invalid dimensions");
    }
}
double HeightAt(const Runway& runway,const glm::dvec3& point) {
    const double x=runway.end.x-runway.start.x,z=runway.end.z-runway.start.z;
    const double along=std::max(0.0,std::min(1.0,((point.x-runway.start.x)*x+(point.z-runway.start.z)*z)/(x*x+z*z)));
    return runway.start.y+(runway.end.y-runway.start.y)*along;
}
bool OnRunway(const Airport& airport,const MissionSample& sample,std::size_t& index,unsigned minimumContacts=2) {
    if(sample.aircraftState!=collision::AircraftState::Grounded||sample.gearContacts<minimumContacts||
       sample.gearCenters.size()!=3||sample.gearRadii.size()!=3)
        return false;
    for(std::size_t i=0;i<airport.runways.size();++i) {
        const auto& runway=airport.runways[i];
        if(!Contains(runway,sample.position,RunwayInset))continue;
        bool inside=true;
        unsigned groundLevelWheels=0;
        for(std::size_t gear=0;gear<sample.gearCenters.size();++gear) {
            const auto& center=sample.gearCenters[gear];
            inside=inside&&Contains(runway,center,RunwayInset);
            // The pivot rises during a safe pitched/banked touchdown. Verify
            // the wheels at runway height instead, while physical contact
            // count/state remains the authority for actual support.
            const double wheelBottom=center.y-sample.gearRadii[gear];
            if(std::abs(wheelBottom-HeightAt(runway,center))<=HeightTolerance)
                ++groundLevelWheels;
        }
        if(inside&&groundLevelWheels>=minimumContacts) {index=i;return true;}
    }
    return false;
}
bool Aligned(const Runway& runway,const glm::dvec3& forward) {
    const double x=runway.end.x-runway.start.x,z=runway.end.z-runway.start.z;
    const double length=std::hypot(x,z),horizontal=std::hypot(forward.x,forward.z);
    if(horizontal<1e-9||!std::isfinite(horizontal))return false;
    return std::abs((forward.x/horizontal)*(x/length)+(forward.z/horizontal)*(z/length))>=AlignmentCosine-1e-12;
}
std::size_t Nearest(const Airport& airport,const glm::dvec3& point,double& distance) {
    std::size_t index=0;distance=std::numeric_limits<double>::infinity();
    for(std::size_t i=0;i<airport.runways.size();++i) {
        const double candidate=DistanceToRunway(airport.runways[i],point);
        if(candidate<distance) {distance=candidate;index=i;}
    }
    return index;
}
}

FlightMission::FlightMission(const Airport& source,const Airport& target):origin(source),destination(target) {
    Validate(origin);Validate(destination);Reset();
}
void FlightMission::Reset() {
    snapshot=MissionSnapshot{};
    previousValid=previousFlying=liftoffPending=previousLandingQualified=arrivalArmed=false;
    originRunway=destinationRunway=previousLandingRunway=arrivalRunway=0;
    airborneSeconds=departureHeight=0;
    snapshot.target=origin.runways.front().end;
    snapshot.targetName=origin.name+" - pista "+origin.runways.front().name;
    snapshot.instruction="Decole do "+origin.name+" para iniciar a missao.";
}
void FlightMission::Guidance(const MissionSample& sample) {
    if(!snapshot.departed) {
        const auto& runway=origin.runways[originRunway];
        snapshot.target=runway.end;
        snapshot.targetName=origin.name+" - pista "+runway.name;
    } else {
        const auto& runway=destination.runways[destinationRunway];
        snapshot.targetName=destination.name+" - pista "+runway.name;
        if(snapshot.stage==Stage::EnRoute) {
            const auto direction=runway.end-runway.start;
            const double length=std::hypot(direction.x,direction.z);
            snapshot.target=runway.start-glm::dvec3(direction.x/length,0,direction.z/length)*1000.0;
        } else snapshot.target=runway.start*.5+runway.end*.5;
    }
    snapshot.distanceMeters=std::hypot(snapshot.target.x-sample.position.x,snapshot.target.z-sample.position.z);
    snapshot.bearingDegrees=BearingToTarget(sample.position,snapshot.target);
}
const MissionSnapshot& FlightMission::Update(const MissionSample& sample,double elapsedSeconds) {
    if(snapshot.stage==Stage::Completed||snapshot.stage==Stage::Failed)return snapshot;
    // An accident always wins over a dwell timer expiring in the same frame.
    if(sample.aircraftState==collision::AircraftState::Crashed) {
        snapshot.stage=Stage::Failed;snapshot.landingSeconds=0;
        snapshot.instruction="Missao falhou: acidente. Enter: reiniciar.";
        return snapshot;
    }
    if(!std::isfinite(elapsedSeconds)||elapsedSeconds<0||!Valid(sample)) {
        previousValid=previousFlying=liftoffPending=previousLandingQualified=arrivalArmed=false;
        airborneSeconds=snapshot.landingSeconds=0;
        if(!snapshot.departed)snapshot.originArmed=false;
        snapshot.stage=snapshot.departed?Stage::EnRoute:Stage::Departure;
        snapshot.instruction="Leitura de voo invalida; aguarde dados validos.";
        return snapshot;
    }
    const bool wasDeparted=snapshot.departed;
    if(wasDeparted)snapshot.flightSeconds+=elapsedSeconds;
    if(!snapshot.departed) {
        if(sample.aircraftState==collision::AircraftState::Grounded) {
            liftoffPending=false;airborneSeconds=0;
            snapshot.originArmed=OnRunway(origin,sample,originRunway);
            if(snapshot.originArmed)departureHeight=HeightAt(origin.runways[originRunway],sample.position);
        } else {
            if(previousValid&&!previousFlying&&snapshot.originArmed) {
                liftoffPending=DistanceToRunway(origin.runways[originRunway],sample.position)<=DepartureObservationMargin;
                if(!liftoffPending)snapshot.originArmed=false;
                airborneSeconds=0;
            } else if(liftoffPending&&previousValid&&previousFlying)airborneSeconds+=elapsedSeconds;
            if(liftoffPending&&airborneSeconds+TimeTolerance>=TakeoffSeconds&&
               sample.position.y-departureHeight>=TakeoffHeight) {
                snapshot.departed=true;snapshot.originArmed=false;snapshot.stage=Stage::EnRoute;
            }
        }
    }
    if(!snapshot.departed) {
        snapshot.stage=Stage::Departure;snapshot.landingSeconds=0;previousLandingQualified=false;
        snapshot.instruction=liftoffPending?"Ganhe 5 m de altura e mantenha o voo para confirmar a decolagem.":
            snapshot.originArmed?"Decole do "+origin.name+" e siga para o "+destination.name+".":
                                  "Posicione o trem de pouso numa pista do "+origin.name+".";
    } else {
        double distance;
        const auto nearest=Nearest(destination,sample.position,distance);
        const bool wasApproaching=snapshot.stage==Stage::Approach||snapshot.stage==Stage::Landing;
        const bool approaching=distance<=2000||(wasApproaching&&distance<=2500);
        std::size_t runway=nearest;
        const bool onDestination=OnRunway(destination,sample,runway);
        // Ground presence alone is not a destination landing. Record the
        // first observed airborne -> supported transition on a particular
        // runway, including a one-wheel touchdown before the gear settles.
        std::size_t touchdownRunway=nearest;
        const bool touchdownFootprint=OnRunway(destination,sample,touchdownRunway,1);
        if(sample.aircraftState==collision::AircraftState::Flying)arrivalArmed=false;
        else if(previousValid&&previousFlying) {
            arrivalArmed=touchdownFootprint;
            if(arrivalArmed)arrivalRunway=touchdownRunway;
        } else if(arrivalArmed&&(!touchdownFootprint||touchdownRunway!=arrivalRunway)) {
            // Taxiing out, crossing to another strip or losing verified
            // support requires a new airborne approach and touchdown.
            arrivalArmed=false;
        }
        destinationRunway=onDestination?runway:approaching?nearest:0;
        if(onDestination&&arrivalArmed&&runway==arrivalRunway) {
            snapshot.stage=Stage::Landing;
            const bool aligned=Aligned(destination.runways[runway],sample.forward);
            const bool slow=std::hypot(sample.velocity.x,sample.velocity.z)<=LandingSpeed+1e-9;
            const bool qualified=aligned&&slow;
            if(qualified&&previousLandingQualified&&previousLandingRunway==runway)
                snapshot.landingSeconds+=elapsedSeconds;
            else snapshot.landingSeconds=0;
            previousLandingQualified=qualified;previousLandingRunway=runway;
            if(!aligned)snapshot.instruction="Alinhe o aviao com a pista.";
            else if(!slow)snapshot.instruction="Reduza para ate 18 km/h na pista.";
            else snapshot.instruction="Mantenha o aviao na pista por 2 s.";
            if(snapshot.landingSeconds+TimeTolerance>=LandingSeconds) {
                snapshot.landingSeconds=LandingSeconds;snapshot.stage=Stage::Completed;
                snapshot.instruction="Missao concluida! Pouso no "+destination.name+" confirmado.";
            }
        } else {
            previousLandingQualified=false;snapshot.landingSeconds=0;
            snapshot.stage=approaching?Stage::Approach:Stage::EnRoute;
            snapshot.instruction=onDestination?"Faca nova aproximacao e pouse na pista.":
                approaching?"Alinhe e pouse com seguranca numa pista do "+destination.name+".":
                            "Siga para o "+destination.name+" pelo alvo de aproximacao.";
        }
    }
    previousValid=true;previousFlying=sample.aircraftState==collision::AircraftState::Flying;
    Guidance(sample);
    return snapshot;
}
} // namespace mission
