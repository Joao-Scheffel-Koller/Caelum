#ifndef CAELUM_FLIGHT_MODEL_HPP
#define CAELUM_FLIGHT_MODEL_HPP
#include "aircraft/Aircraft.hpp"
#include "aircraft/FlightControls.hpp"

// International Standard Atmosphere (troposphere, below 11 km).
struct Atmosphere {
    float density;      // kg/m^3
    float soundSpeed;   // m/s
};
Atmosphere StandardAtmosphere(float altitudeMeters);

// Aerodynamic coefficients of the current configuration at angle `alpha`.
float LiftCoefficient(const FlightProfile& p,const FlapSetting& flaps,float alpha);
float DragCoefficient(const FlightProfile& p,const FlapSetting& flaps,float alpha,float mach);
// Total engine thrust (N) at the given throttle, airspeed and altitude.
float EngineThrust(const FlightProfile& p,float throttle,float speed,float altitude);

class FlightModel{

    private:
        float gravity;

    public:
        FlightModel();
        // AI-assisted: forces (thrust, lift, drag, weight, tyre friction) and
        // the load-factor control law. `grounded` comes from the previous
        // contact resolution; `input` is the mouse yoke.
        void update(Aircraft* a, const flightcontrols::ControlInput& input, bool grounded, float dt);
        void update(Aircraft* a, float dt); // No pilot input, airborne.
        void increase_throttle(Aircraft* a);
        void decrease_throttle(Aircraft* a);
        void extend_flaps(Aircraft* a);
        void retract_flaps(Aircraft* a);
        float get_gravity() const {return gravity;}
};
#endif
