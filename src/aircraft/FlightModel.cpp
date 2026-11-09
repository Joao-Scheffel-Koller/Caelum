#include "aircraft/FlightModel.hpp"
#include "aircraft/Aircraft.hpp"
#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <glm/geometric.hpp>

namespace {
const float MinimumAirspeed=1.0f;   // m/s: below it angles of attack are meaningless.
const float PathHoldGain=0.3f;      // 1/s: flight path correction with a neutral stick.

float Clamp(float v,float lo,float hi) {return std::max(lo,std::min(hi,v));}

// Load factor that keeps the flight path angle with a neutral stick (Airbus
// normal law): cos(gamma)/cos(bank), with bank compensation only up to 33 deg.
float NeutralLoadFactor(float gamma,float bank) {
    const float c=std::cos(bank),limit=std::cos(33.0f*0.01745329f);
    const float bankTerm=std::fabs(c)>=limit?1.0f/c:c/(limit*limit); // Continuous at 33 deg.
    return std::cos(gamma)*bankTerm;
}
} // namespace

Atmosphere StandardAtmosphere(float altitude) {
    // FONTE: ICAO Standard Atmosphere (Doc 7488): T = 288.15 - 0.0065 h,
    // rho = 1.225 (T/288.15)^4.2559, a = sqrt(1.4 * 287.05 * T).
    const float h=Clamp(altitude,-500.0f,11000.0f);
    const float temperature=288.15f-0.0065f*h;
    Atmosphere air;
    air.density=1.225f*std::pow(temperature/288.15f,4.2559f);
    air.soundSpeed=std::sqrt(1.4f*287.05f*temperature);
    return air;
}

float LiftCoefficient(const FlightProfile& p,const FlapSetting& flaps,float alpha) {
    const float cl0=p.CL0+flaps.deltaCL0;
    const float stall=(flaps.maxCL-cl0)/p.CLalpha;
    const float negativeStall=(p.minCL-cl0)/p.CLalpha;
    if(alpha>=negativeStall&&alpha<=stall)return cl0+p.CLalpha*alpha; // Linear region.
    // Beyond the stall the lift falls, but never below the flat-plate value.
    const float flatPlate=std::sin(2.0f*alpha);
    if(alpha>stall)return std::max(flaps.maxCL-p.stallDrop*(alpha-stall),flatPlate);
    return std::min(p.minCL+p.stallDrop*(negativeStall-alpha),flatPlate);
}

float DragCoefficient(const FlightProfile& p,const FlapSetting& flaps,float alpha,float mach) {
    const float cl=LiftCoefficient(p,flaps,alpha);
    // Drag polar: parasite + induced (k CL^2).
    float cd=p.CD0+flaps.deltaCD0+p.InducedDragFactor()*cl*cl;
    // Lock's approximation of the transonic drag rise: 20 (M - Mcrit)^4.
    if(mach>p.criticalMach)cd+=20.0f*std::pow(mach-p.criticalMach,4.0f);
    // Stalled wing: approaches flat-plate drag 1.28 sin^2(alpha).
    const float s=std::sin(alpha);
    return std::max(cd,1.28f*s*s);
}

float EngineThrust(const FlightProfile& p,float throttle,float speed,float altitude) {
    const float sigma=StandardAtmosphere(altitude).density/1.225f;
    const float t=Clamp(throttle,0.0f,1.0f);
    if(p.propulsion==Propulsion::Propeller) {
        // Constant power: T = eta P / V, limited by the static thrust.
        const float available=speed>1.0f?p.propEfficiency*p.enginePower/speed:p.maxThrust;
        return t*std::min(p.maxThrust,available)*sigma;
    }
    // High-bypass turbofan lapse (Mattingly, "Aircraft Engine Design"):
    // T/T0 = sigma (1 - 0.49 sqrt(M)). ~78% of static thrust at takeoff speed,
    // ~18% at FL350 / M0.78 (CFM56: ~22 kN per engine in cruise).
    const float mach=speed/StandardAtmosphere(altitude).soundSpeed;
    return t*p.maxThrust*sigma*std::max(0.0f,1.0f-0.49f*std::sqrt(mach));
}

FlightModel :: FlightModel(){
    // Standard gravity (m/s^2); the world is in meters.
    gravity = -9.80665f;
}

void FlightModel::update(Aircraft* aircraft, float dt)
{
    update(aircraft,flightcontrols::ControlInput(),false,dt);
}

void FlightModel::update(Aircraft* aircraft, const flightcontrols::ControlInput& input, bool grounded, float dt)
{
    if(!(dt>0.0f)||!std::isfinite(dt))return;
    const FlightProfile& p=aircraft->get_profile();
    const FlapSetting& flaps=aircraft->get_flaps();
    const float mass=aircraft->get_mass();
    const float g=-gravity;

    glm::vec3 forward=glm::normalize(aircraft->get_forward());
    glm::vec3 right=glm::normalize(glm::cross(forward,aircraft->get_up()));
    glm::vec3 velocity=aircraft->get_velocity();
    glm::vec3 position=aircraft->get_position();
    //.length() is not used because it was always rounding the number up to 3.0f
    float speed = std::sqrt(
    velocity.x * velocity.x +
    velocity.y * velocity.y +
    velocity.z * velocity.z
    );

    // 1. Atmosfera (altitude = Y do mapa, nivel do mar em Y=0).
    const Atmosphere air=StandardAtmosphere(position.y);
    const float dynamicPressure=0.5f*air.density*speed*speed;
    const float mach=speed/air.soundSpeed;

    // 2. Angulo de ataque.
    glm::vec3 airDirection=forward;
    glm::vec3 liftDirection=glm::cross(right,forward);
    float alpha=aircraft->get_alpha();
    if(speed>MinimumAirspeed) {
        airDirection=velocity/speed;
        // Lift is perpendicular to the airflow, inside the plane of symmetry.
        glm::vec3 sideAxis=right-airDirection*glm::dot(right,airDirection);
        if(glm::length(sideAxis)>1e-4f)liftDirection=glm::normalize(glm::cross(glm::normalize(sideAxis),airDirection));
        const glm::vec3 up=glm::cross(right,forward);
        const float geometric=std::atan2(-glm::dot(airDirection,up),glm::dot(airDirection,forward));
        if(grounded) {
            alpha=geometric; // Wheels fix the attitude: alpha follows the body.
            aircraft->set_path_hold(false,0.0f);
        } else {
            // Load-factor demand (sidestick): neutral keeps the flight path,
            // full deflection reaches the structural limits of the profile.
            const bool flapsOut=aircraft->get_flap_index()>0;
            const float nMax=flapsOut?p.maxLoadFactorFlaps:p.maxLoadFactor;
            const float nMin=flapsOut?p.minLoadFactorFlaps:p.minLoadFactor;
            const float gamma=std::asin(Clamp(airDirection.y,-1.0f,1.0f));
            const float bank=flightcontrols::RollAngle(forward,up);
            float neutral=NeutralLoadFactor(gamma,bank);
            const float command=Clamp(input.pitch,-1.0f,1.0f);
            // Flight path stability: when the stick returns to neutral, the
            // current path angle becomes the reference and small drifts
            // (acceleration, alpha lag) are corrected: dn = K (gamma_ref - gamma) V / g.
            if(command!=0.0f)aircraft->set_path_hold(false,0.0f);
            else if(!aircraft->get_path_hold())aircraft->set_path_hold(true,gamma);
            else if(std::fabs(bank)<1.5f)
                neutral+=PathHoldGain*(aircraft->get_path_reference()-gamma)*speed/g;
            const float n=command>=0?neutral+command*(nMax-neutral):neutral+command*(neutral-nMin);
            // Angle of attack that produces that lift: L = n m g = q S CL.
            const float cl0=p.CL0+flaps.deltaCL0;
            const float requiredCL=dynamicPressure>1.0f?n*mass*g/(dynamicPressure*p.wingArea):flaps.maxCL;
            const float stall=(flaps.maxCL-cl0)/p.CLalpha;
            const float negativeStall=(p.minCL-cl0)/p.CLalpha;
            // Alpha protection: the law never commands a stall.
            const float target=Clamp((requiredCL-cl0)/p.CLalpha,
                negativeStall+p.alphaProtectionMargin,stall-p.alphaProtectionMargin);
            // Short-period response: first-order lag toward the target.
            alpha+=(target-alpha)*(1.0f-std::exp(-dt/p.alphaTimeConstant));
        }
    } else if(grounded) {
        alpha=std::atan2(forward.y,std::hypot(forward.x,forward.z));
    }
    aircraft->set_alpha(alpha);

    // 3. Forcas.
    const float cl=LiftCoefficient(p,flaps,alpha);
    const float cd=DragCoefficient(p,flaps,alpha,mach);
    const float liftMagnitude=dynamicPressure*p.wingArea*cl;
    const glm::vec3 liftForce=liftDirection*liftMagnitude;
    const glm::vec3 dragForce=-airDirection*(dynamicPressure*p.wingArea*cd)*(speed>MinimumAirspeed?1.0f:0.0f);
    const glm::vec3 thrustForce=forward*EngineThrust(p,aircraft->get_throttle(),speed,position.y);
    const glm::vec3 gravityForce(0.0f,gravity*mass,0.0f);
    const glm::vec3 totalForce=thrustForce+gravityForce+liftForce+dragForce;

    // 4. Segunda lei de Newton e integracao (Euler semi-implicito).
    const glm::vec3 acceleration=totalForce/mass;
    aircraft->set_acceleration(acceleration);
    velocity+=acceleration*dt;

    // 5. Atrito dos pneus: opposes horizontal motion with mu * normal force,
    // never reversing it (it can stop the airplane, not push it backward).
    if(grounded) {
        const float normal=std::max(0.0f,mass*g-liftForce.y);
        const float mu=p.rollingFriction+(aircraft->get_brakes()?p.brakeFriction:0.0f);
        glm::vec3 horizontal(velocity.x,0.0f,velocity.z);
        const float horizontalSpeed=glm::length(horizontal);
        if(horizontalSpeed>0.0f) {
            const float reduced=std::max(0.0f,horizontalSpeed-mu*normal/mass*dt);
            horizontal*=reduced/horizontalSpeed;
            velocity.x=horizontal.x;velocity.z=horizontal.z;
        }
    }
    aircraft->set_velocity(velocity);

    // 6. Atualiza posicao.
    position += velocity * dt;
    // AI-assisted: AircraftContacts resolves real-map contacts after this
    // prediction. Preserve the impact velocity until that classification.
    aircraft->set_position(position);

    // 7. Em voo, o corpo acompanha o vento relativo: nose = airflow rotated
    // by alpha, zero sideslip (coordinated flight). The bank chosen by the
    // pilot (right axis) is preserved; the turn itself comes from the tilted lift.
    const float newSpeed=glm::length(velocity);
    if(!grounded&&newSpeed>MinimumAirspeed) {
        const glm::vec3 direction=velocity/newSpeed;
        glm::vec3 side=right-direction*glm::dot(right,direction);
        if(glm::length(side)>1e-4f) {
            side=glm::normalize(side);
            const glm::vec3 windUp=glm::cross(side,direction);
            const glm::vec3 nose=glm::normalize(direction*std::cos(alpha)+windUp*std::sin(alpha));
            aircraft->set_forward(nose);
            aircraft->set_right(side);
            aircraft->set_up(glm::cross(side,nose));
        }
    }
}

void FlightModel :: increase_throttle(Aircraft* a)
{
    float new_throttle = std::fmin(a->get_throttle() + a->get_throttle_increase_rate(), a->get_max_throttle());
    a->set_throttle(new_throttle);
    printf("throttle: %f\n", new_throttle);
}
void FlightModel ::  decrease_throttle(Aircraft* a)
{
    float new_throttle = std::fmax(a->get_throttle() - a->get_throttle_increase_rate(), 0.0f);
    a->set_throttle(new_throttle);
    printf("throttle: %f\n", new_throttle);
}
void FlightModel::extend_flaps(Aircraft* a)
{
    a->set_flap_index(a->get_flap_index()+1);
    printf("flaps: %s\n",a->get_flaps().name.c_str());
}
void FlightModel::retract_flaps(Aircraft* a)
{
    a->set_flap_index(a->get_flap_index()-1);
    printf("flaps: %s\n",a->get_flaps().name.c_str());
}
