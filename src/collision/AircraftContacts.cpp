#include "collision/AircraftContacts.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace collision {
namespace {
const double Pi=3.14159265358979323846;
// Gameplay thresholds for the simplified simulator, not aircraft certification.
const double MaximumSinkSpeed=3.0;
const double MaximumPitchDegrees=15.0;
const double MaximumRollDegrees=10.0;
const double MinimumSupportNormal=.85;
const double SupportProbeDistance=.08;
const double MaximumRotationStep=5.0*Pi/180;
const double ContactTolerance=1e-6;

bool Finite(const glm::dvec3& v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct Quaternion {double w,x,y,z;};
Quaternion Unit(Quaternion q) {
    const double length=std::sqrt(q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z);
    return {q.w/length,q.x/length,q.y/length,q.z/length};
}
double Dot(Quaternion a,Quaternion b) {return a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;}
Quaternion Frame(const AircraftPose& pose) {
    if(!Finite(pose.position)||!Finite(pose.forward)||!Finite(pose.up)||
       glm::length(pose.forward)<1e-10||glm::length(pose.up)<1e-10)
        throw std::invalid_argument("Invalid aircraft contact pose");
    const auto f=glm::normalize(pose.forward);
    auto r=glm::cross(f,glm::normalize(pose.up));
    if(glm::length(r)<1e-10)throw std::invalid_argument("Parallel aircraft contact axes");
    r=glm::normalize(r);
    const auto u=glm::cross(r,f);
    // Our own basis-to-quaternion conversion; no external transform builders.
    Quaternion q;
    const double trace=f.x+u.y+r.z;
    if(trace>0) {
        const double s=2*std::sqrt(trace+1);
        q={s*.25,(u.z-r.y)/s,(r.x-f.z)/s,(f.y-u.x)/s};
    } else if(f.x>u.y&&f.x>r.z) {
        const double s=2*std::sqrt(1+f.x-u.y-r.z);
        q={(u.z-r.y)/s,s*.25,(u.x+f.y)/s,(r.x+f.z)/s};
    } else if(u.y>r.z) {
        const double s=2*std::sqrt(1+u.y-f.x-r.z);
        q={(r.x-f.z)/s,(u.x+f.y)/s,s*.25,(r.y+u.z)/s};
    } else {
        const double s=2*std::sqrt(1+r.z-f.x-u.y);
        q={(f.y-u.x)/s,(r.x+f.z)/s,(r.y+u.z)/s,s*.25};
    }
    return Unit(q);
}
glm::dvec3 Rotate(Quaternion q,const glm::dvec3& v) {
    const glm::dvec3 imaginary(q.x,q.y,q.z);
    const auto t=2.0*glm::cross(imaginary,v);
    return v+q.w*t+glm::cross(imaginary,t);
}
Quaternion Interpolate(Quaternion a,Quaternion b,double t) {
    double dot=Dot(a,b);
    if(dot<0) {b={-b.w,-b.x,-b.y,-b.z};dot=-dot;}
    double left=1-t,right=t;
    if(dot<.9995) {
        const double angle=std::acos(std::max(-1.0,std::min(1.0,dot)));
        left=std::sin((1-t)*angle)/std::sin(angle);
        right=std::sin(t*angle)/std::sin(angle);
    }
    return Unit({left*a.w+right*b.w,left*a.x+right*b.x,left*a.y+right*b.y,left*a.z+right*b.z});
}
struct MovingFrame {glm::dvec3 position;Quaternion orientation;};
MovingFrame Between(const AircraftPose& a,const AircraftPose& b,Quaternion qa,Quaternion qb,double t) {
    return {a.position*(1-t)+b.position*t,Interpolate(qa,qb,t)};
}
glm::dvec3 Center(const MovingFrame& frame,const ProbeSphere& sphere) {
    return frame.position+Rotate(frame.orientation,sphere.center);
}
void Pose(ContactResult& result,const MovingFrame& frame) {
    result.position=frame.position;
    result.forward=Rotate(frame.orientation,glm::dvec3(1,0,0));
    result.up=Rotate(frame.orientation,glm::dvec3(0,1,0));
}
bool Earlier(const Hit& candidate,const Hit& best) {
    return candidate.hit&&(!best.hit||candidate.time<best.time-1e-9||
        (std::abs(candidate.time-best.time)<=1e-9&&
         candidate.surface!=Surface::Ground&&best.surface==Surface::Ground));
}
bool SafeAttitude(const MovingFrame& frame) {
    const auto f=Rotate(frame.orientation,glm::dvec3(1,0,0));
    const auto u=Rotate(frame.orientation,glm::dvec3(0,1,0));
    const double horizontal=std::hypot(f.x,f.z);
    const double pitch=std::atan2(f.y,horizontal)*180/Pi;
    if(std::abs(pitch)>MaximumPitchDegrees||horizontal<1e-8)return false;
    const auto right=glm::normalize(glm::cross(f,glm::dvec3(0,1,0)));
    const auto up=glm::cross(right,f);
    const double roll=std::atan2(glm::dot(u,right),glm::dot(u,up))*180/Pi;
    return std::abs(roll)<=MaximumRollDegrees;
}
CrashReason ReasonFor(const Hit& hit,bool gear) {
    if(hit.surface==Surface::Water)return CrashReason::Water;
    if(hit.surface==Surface::Ground&&!gear)return CrashReason::AirframeContact;
    return CrashReason::Obstacle;
}
}

const char* CrashReasonText(CrashReason why) {
    switch(why) {
    case CrashReason::None:return "";
    case CrashReason::HardLanding:return "Impacto forte no solo";
    case CrashReason::UnsafeAttitude:return "Inclinacao insegura no pouso";
    case CrashReason::Obstacle:return "Colisao com obstaculo";
    case CrashReason::Water:return "Colisao com agua";
    case CrashReason::AirframeContact:return "Fuselagem, asa ou cauda no solo";
    case CrashReason::InvalidPose:return "Estado de voo invalido";
    }
    return "Acidente";
}
void AircraftContacts::Reset() {
    state=AircraftState::Flying;reason=CrashReason::None;frozen=ContactResult{};
}
void AircraftContacts::Configure(const glm::dvec3& low,const glm::dvec3& high,AirframeType type) {
    const auto size=high-low;
    if(!Finite(low)||!Finite(high)||size.x<=0||size.y<=0||size.z<=0)
        throw std::invalid_argument("Invalid aircraft model bounds");
    localSpheres.clear();
    const glm::dvec3 pivot((low.x+high.x)*.5,low.y,(low.z+high.z)*.5);
    const auto local=[&](double x,double y,double z) {
        return glm::dvec3(low.x+x*size.x,low.y+y*size.y,pivot.z+z*size.z)-pivot;
    };
    const auto chain=[&](glm::dvec3 from,glm::dvec3 to,double radius) {
        // Overlapping spheres approximate each capsule. Dense overlap avoids
        // the empty space of a single bounding box around the whole airplane.
        const int count=std::max(1,int(std::ceil(glm::length(to-from)/(radius*1.3))));
        for(int i=0;i<=count;++i)
            localSpheres.push_back({from+(to-from)*(double(i)/count),radius,false});
    };
    if(type==AirframeType::LightTaildragger) {
        // FONTE: geometry survey of the CC BY MagicGames mesh after conversion;
        // exact wheel measurements and derivation are in data/light_aircraft/metadata.json.
        chain(local(.10,.50,0),local(.91,.50,0),size.x*.05);
        for(double side:{-1.0,1.0}) {
            chain(local(.68,.82,side*.02),local(.63,.82,side*.49),size.x*.013);
            chain(local(.12,.61,side*.02),local(.12,.61,side*.18),size.x*.012);
        }
        chain(local(.10,.57,0),local(.10,.96,0),size.x*.012);
        for(double side:{-1.0,1.0})
            localSpheres.push_back({local(.8417722,.1104324,side*.1543546),size.x*.03681081,true});
        localSpheres.push_back({local(.1600221,.07287955,-.00882703),size.x*.02199480,true});
        Reset();return;
    }
    // Proportions surveyed from this A320: nose +X, wings sweep backward, wheels
    // near model X=5.5 (nose) and -7.5/Z=+-3.3 (main), minY=-3.342473.
    // AI-assisted (physics branch): the fuselage follows the mesh underside
    // instead of one constant capsule: belly ~1.49 m above the wheels from the
    // nose to X=-14, then the tail cone sweeps up (2.06 m at X=-20.5, ~3.5 m at
    // X=-25). This gives the mesh's own tail-strike pitch (~9 deg about the
    // main gear) instead of ~3 deg, so a normal takeoff rotation is possible.
    const auto tapered=[&](glm::dvec3 from,glm::dvec3 to,double r0,double r1) {
        const int count=std::max(1,int(std::ceil(glm::length(to-from)/(std::min(r0,r1)*1.3))));
        for(int i=0;i<=count;++i) {
            const double t=double(i)/count;
            localSpheres.push_back({from+(to-from)*t,r0+(r1-r0)*t,false});
        }
    };
    chain(local(.35354,.3003,0),local(.94,.3003,0),size.x*.052);
    tapered(local(.18236,.30836,0),local(.35354,.3003,0),size.x*.0395,size.x*.052);
    tapered(local(.06386,.3727,0),local(.18236,.30836,0),size.x*.02107,size.x*.0395);
    for(double side:{-1.0,1.0}) {
        chain(local(.56,.23,side*.025),local(.45,.31,side*.49),size.x*.018);
        chain(local(.10,.43,side*.025),local(.04,.43,side*.18),size.x*.012);
        chain(local(.47,.165,side*.16),local(.59,.165,side*.16),size.x*.028);
    }
    chain(local(.075,.43,0),local(.075,.94,0),size.x*.016);
    const double wheelRadius=size.x*.0065;
    for(const auto& point:{local(.87,0,0),local(.525,0,-.095),local(.525,0,.095)})
        localSpheres.push_back({point+glm::dvec3(0,wheelRadius,0),wheelRadius,true});
    Reset();
}
std::vector<ProbeSphere> AircraftContacts::DebugSpheres(const AircraftPose& pose) const {
    const auto orientation=Frame(pose);
    std::vector<ProbeSphere> result;
    result.reserve(localSpheres.size());
    for(const auto& sphere:localSpheres)
        result.push_back({pose.position+Rotate(orientation,sphere.center),sphere.radius,sphere.gear});
    return result;
}
ContactResult AircraftContacts::Crash(ContactResult result,CrashReason why,const Hit& hit) {
    state=result.state=AircraftState::Crashed;reason=result.reason=why;
    result.contact=true;result.hit=hit;result.velocity=glm::dvec3(0);
    frozen=result;
    return result;
}
ContactResult AircraftContacts::Resolve(const CollisionWorld& world,const AircraftPose& oldPose,
    const AircraftPose& newPose,const glm::dvec3& velocity,double dt) {
    if(state==AircraftState::Crashed)return frozen;
    if(localSpheres.empty())throw std::logic_error("Configure aircraft collision volumes before Resolve");
    ContactResult result;
    Quaternion qa,qb;
    try {
        qa=Frame(oldPose);qb=Frame(newPose);
        if(!Finite(velocity)||!std::isfinite(dt)||dt<0||!Finite(newPose.position-oldPose.position))
            throw std::invalid_argument("Invalid aircraft contact step");
    } catch(const std::invalid_argument&) {
        result.position=Finite(oldPose.position)?oldPose.position:glm::dvec3(0);
        return Crash(result,CrashReason::InvalidPose,Hit{});
    }
    result.velocity=velocity;
    Pose(result,{newPose.position,qb});
    const double angle=2*std::acos(std::min(1.0,std::abs(Dot(qa,qb))));
    const int steps=std::max(1,int(std::ceil(angle/MaximumRotationStep)));
    glm::dvec3 correction(0);
    bool previouslySupported=state==AircraftState::Grounded;
    for(int step=0;step<steps;++step) {
        const double t0=double(step)/steps,t1=double(step+1)/steps;
        auto start=Between(oldPose,newPose,qa,qb,t0),end=Between(oldPose,newPose,qa,qb,t1);
        start.position+=correction;end.position+=correction;
        Hit support,danger;
        bool dangerousGear=false;
        size_t supportingSphere=0;
        for(size_t i=0;i<localSpheres.size();++i) {
            const auto& sphere=localSpheres[i];
            const auto from=Center(start,sphere),to=Center(end,sphere);
            const auto hit=world.SweepSphere(from,to,sphere.radius);
            if(!hit.hit)continue;
            if(sphere.gear&&hit.surface==Surface::Ground&&hit.normal.y>=MinimumSupportNormal) {
                // A wheel departing the ground must not reattach on its initial
                // tangency. Parallel/downward contact can support taxi normally.
                if(glm::dot(to-from,hit.normal)>ContactTolerance&&hit.penetration<ContactTolerance) {
                    // Leaving this floor contact does not make the rest of a
                    // wheel's path safe: a low obstacle may still lie ahead.
                    const auto ahead=world.SweepSphere(from,to,sphere.radius,nullptr,true);
                    if(Earlier(ahead,danger)) {danger=ahead;dangerousGear=true;}
                    continue;
                }
                bool deeper=false;
                if(support.hit&&std::abs(hit.time-support.time)<=1e-9) {
                    const auto& previous=localSpheres[supportingSphere];
                    const double depth=(sphere.radius-glm::dot(to-hit.point,hit.normal))/hit.normal.y;
                    const double previousDepth=(previous.radius-glm::dot(Center(end,previous)-support.point,
                                                                         support.normal))/support.normal.y;
                    deeper=depth>previousDepth;
                }
                if(Earlier(hit,support)||deeper) {support=hit;supportingSphere=i;}
            } else if(Earlier(hit,danger)) {danger=hit;dangerousGear=sphere.gear;}
        }
        if(danger.hit&&(!support.hit||danger.time<=support.time+1e-9)) {
            auto impact=Between(oldPose,newPose,qa,qb,t0+(t1-t0)*danger.time);
            impact.position+=correction;Pose(result,impact);
            return Crash(result,ReasonFor(danger,dangerousGear),danger);
        }
        if(support.hit) {
            auto impact=Between(oldPose,newPose,qa,qb,t0+(t1-t0)*support.time);
            impact.position+=correction;
            if(velocity.y < -MaximumSinkSpeed) {
                Pose(result,impact);return Crash(result,CrashReason::HardLanding,support);
            }
            if(!SafeAttitude(impact)) {
                Pose(result,impact);return Crash(result,CrashReason::UnsafeAttitude,support);
            }
            const auto& wheel=localSpheres[supportingSphere];
            // Start the remaining path at touchdown, not at the original
            // airborne pose. Initial overlap also needs depenetration here.
            const double impactDepth=wheel.radius-glm::dot(Center(impact,wheel)-support.point,support.normal);
            impact.position.y+=std::max(0.0,impactDepth/support.normal.y);
            const double depth=wheel.radius-glm::dot(Center(end,wheel)-support.point,support.normal);
            const double lift=std::max(0.0,depth/support.normal.y);
            correction.y+=lift;end.position.y+=lift;
            if(result.velocity.y<0)result.velocity.y=0;
            result.contact=true;result.hit=support;
            previouslySupported=true;
            // Re-test the remaining motion at the supported height: an earlier
            // wheel landing must prevent the body's predicted fall into soil,
            // but must not hide a later wall/water contact along the taxi path.
            // A danger after touchdown on the original descending path is
            // obsolete. Query touchdown -> corrected end instead. Supported
            // wheels skip upward ground candidates inside the spatial query,
            // so an initial floor contact cannot hide a later low obstacle.
            danger=Hit{};
            for(const auto& sphere:localSpheres) {
                const auto hit=world.SweepSphere(Center(impact,sphere),Center(end,sphere),sphere.radius,nullptr,sphere.gear);
                if(!hit.hit)continue;
                if(Earlier(hit,danger)) {danger=hit;dangerousGear=sphere.gear;}
            }
            if(danger.hit) {
                const double fraction=support.time+(1-support.time)*danger.time;
                const double t=t0+(t1-t0)*fraction;
                auto stopped=Between(oldPose,newPose,qa,qb,t);
                stopped.position=impact.position*(1-danger.time)+end.position*danger.time;
                Pose(result,stopped);return Crash(result,ReasonFor(danger,dangerousGear),danger);
            }
        }
        result.gearContacts=0;
        double supportLift=-SupportProbeDistance;
        if(previouslySupported&&result.velocity.y<=.1) {
            for(const auto& sphere:localSpheres)if(sphere.gear) {
                const auto center=Center(end,sphere);
                const auto hit=world.SweepSphere(center+glm::dvec3(0,.01,0),
                    center-glm::dvec3(0,SupportProbeDistance,0),sphere.radius);
                if(hit.hit&&(hit.surface==Surface::Water||hit.surface==Surface::Solid)) {
                    Pose(result,end);return Crash(result,ReasonFor(hit,true),hit);
                }
                if(hit.hit&&hit.surface==Surface::Ground&&hit.normal.y>=MinimumSupportNormal) {
                    ++result.gearContacts;
                    const double lift=(sphere.radius-glm::dot(center-hit.point,hit.normal))/hit.normal.y;
                    if(std::abs(lift)<=SupportProbeDistance+.011)supportLift=std::max(supportLift,lift);
                }
            }
        }
        if(result.gearContacts) {
            end.position.y+=supportLift;correction.y+=supportLift;
            if(!SafeAttitude(end)) {
                Pose(result,end);return Crash(result,CrashReason::UnsafeAttitude,result.hit);
            }
            if(result.velocity.y<0)result.velocity.y=0;
            result.contact=true;state=AircraftState::Grounded;
        } else state=AircraftState::Flying;
        previouslySupported=state==AircraftState::Grounded;
        Pose(result,end);
    }
    result.state=state;result.reason=reason=CrashReason::None;
    return result;
}
} // namespace collision
