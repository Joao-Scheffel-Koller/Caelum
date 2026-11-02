#include "mission/Airports.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace mission {
namespace {
const double BoundaryTolerance=1e-7;
bool Finite(const glm::dvec3& value) {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}
struct LocalPoint {double along,across,length;};
bool Project(const Runway& runway,const glm::dvec3& point,LocalPoint& local) {
    if(!Finite(runway.start)||!Finite(runway.end)||!Finite(point)||
       !std::isfinite(runway.width)||runway.width<=0)return false;
    const double dx=runway.end.x-runway.start.x,dz=runway.end.z-runway.start.z;
    const double length=std::hypot(dx,dz);
    if(!std::isfinite(length)||length<=1e-9)return false;
    const double x=point.x-runway.start.x,z=point.z-runway.start.z;
    local.along=x*(dx/length)+z*(dz/length);
    local.across=x*(-dz/length)+z*(dx/length);
    local.length=length;
    return std::isfinite(local.along)&&std::isfinite(local.across);
}
}

const Airport& SantosDumont() {
    // FONTE: OpenStreetMap contributors, ODbL. Ways 510811286/12292143 from
    // the original rio.osm used by OSM2World; provenance in data/rio/airports.json.
    // Endpoints are ordered north -> south, matching the existing 20L spawn.
    static const Airport airport={"SBRJ","Santos Dumont",{
        {"SBRJ-20L-02R","20L/02R",{5231.293756270290,0,6195.886840177295},
                                  {5309.545170669083,0,7519.975926467563},42},
        {"SBRJ-20R-02L","20R/02L",{5154.355422762664,0,6199.929394526560},
                                  {5229.498528487885,0,7463.976737845704},30}}};
    return airport;
}
const Airport& Galeao() {
    // FONTE: same OSM extract, ways 12291116/12291112. Displaced-threshold
    // ways are excluded; the mission accepts the main runway rectangles only.
    // The first runway starts at the southeastern end closest to Santos Dumont.
    static const Airport airport={"SBGL","Galeao",{
        {"SBGL-15-33","15/33",{-2591.026611413025,0,-2278.171223196737},
                               {-4971.293481238514,0,-3990.600752469027},47},
        {"SBGL-10-28","10/28",{-4204.833802839074,0,-5243.320168585817},
                               {-353.434187243697,0,-6318.690257219407},45}}};
    return airport;
}
bool Contains(const Runway& runway,const glm::dvec3& point,double inset) {
    LocalPoint local;
    if(!std::isfinite(inset)||inset<0||!Project(runway,point,local)||
       2*inset>=runway.width||2*inset>=local.length)return false;
    return local.along>=inset-BoundaryTolerance&&
           local.along<=local.length-inset+BoundaryTolerance&&
           std::abs(local.across)<=runway.width*.5-inset+BoundaryTolerance;
}
double DistanceToRunway(const Runway& runway,const glm::dvec3& point) {
    LocalPoint local;
    if(!Project(runway,point,local))return std::numeric_limits<double>::infinity();
    const double longitudinal=std::max(0.0,std::max(-local.along,local.along-local.length));
    const double lateral=std::max(0.0,std::abs(local.across)-runway.width*.5);
    return std::hypot(longitudinal,lateral);
}
double BearingToTarget(const glm::dvec3& from,const glm::dvec3& to) {
    if(!Finite(from)||!Finite(to))throw std::invalid_argument("Nonfinite airport bearing position");
    const double dx=to.x-from.x,dz=to.z-from.z;
    if(!std::isfinite(dx)||!std::isfinite(dz))throw std::invalid_argument("Nonfinite airport bearing displacement");
    if(dx==0&&dz==0)return 0;
    double angle=std::atan2(dx,-dz)*180/std::acos(-1.0);
    if(angle<0)angle+=360;
    return angle==0||angle>=360?0:angle;
}
} // namespace mission
