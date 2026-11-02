#include "ui/HudNeedleMotion.hpp"
#include <cmath>

namespace {
const double ResponseSeconds=.12;

double Heading(double degrees) {
    double result=std::fmod(degrees,360.0);
    if(result<0)result+=360;
    return result==0?0.0:result;
}

void Channel(double& current,bool& valid,double target,bool targetValid,
             double alpha,bool circular=false) {
    if(!targetValid||!std::isfinite(target)) {
        current=0;
        valid=false;
        return;
    }
    if(circular)target=Heading(target);
    if(!valid||alpha>=1)current=target;
    else if(alpha>0) {
        if(circular)current=Heading(current+alpha*std::remainder(target-current,360.0));
        else {
            // Weighted endpoints also remain finite for opposite large values,
            // where computing target-current first could overflow.
            const double blended=(1-alpha)*current+alpha*target;
            current=std::isfinite(blended)?blended:target;
        }
    }
    valid=true;
}
}

const FlightTelemetry& HudNeedleMotion::Update(const FlightTelemetry& target,double elapsedSeconds) {
    // Exact first-order response to a held target, independent of frame rate.
    // Invalid/nonpositive elapsed time freezes existing valid needle readings.
    const double alpha=std::isfinite(elapsedSeconds)&&elapsedSeconds>0?
        -std::expm1(-elapsedSeconds/ResponseSeconds):0;
    Channel(displayed.speedKmh,displayed.speedValid,target.speedKmh,target.speedValid,alpha);
    Channel(displayed.throttlePercent,displayed.throttleValid,target.throttlePercent,target.throttleValid,alpha);
    Channel(displayed.altitudeMeters,displayed.altitudeValid,target.altitudeMeters,target.altitudeValid,alpha);
    Channel(displayed.headingDegrees,displayed.headingValid,target.headingDegrees,target.headingValid,alpha,true);

    displayed.attitudeValid=target.attitudeValid&&std::isfinite(target.pitchDegrees)&&std::isfinite(target.rollDegrees);
    displayed.pitchDegrees=displayed.attitudeValid?target.pitchDegrees:0;
    displayed.rollDegrees=displayed.attitudeValid?target.rollDegrees:0;
    return displayed;
}
