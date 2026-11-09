#ifndef CAELUM_FLIGHT_PROFILE_HPP
#define CAELUM_FLIGHT_PROFILE_HPP
// AI-assisted: physical parameters of each selectable aircraft. SI units
// (kg, m, N, W, rad, s). Sources and derivations are listed in docs/physics.md.
#include <string>
#include <vector>

struct FlapSetting {
    std::string name;   // Label shown on the HUD ("0", "1+F", "FULL"...).
    float deltaCL0;     // Lift coefficient added at zero angle of attack.
    float maxCL;        // Maximum lift coefficient (stall) in this setting.
    float deltaCD0;     // Parasite drag added by the deflected flaps/slats.
};

enum class Propulsion { Turbofan, Propeller };

struct FlightProfile {
    std::string id;
    // Mass and geometry.
    float mass;              // kg
    float wingArea;          // m^2 (reference area S)
    float wingSpan;          // m
    // Aerodynamics: CL = CL0 + CLalpha*alpha; CD = CD0 + k*CL^2 (+ wave drag).
    float CL0;               // Clean lift coefficient at alpha=0 (camber + wing incidence).
    float CLalpha;           // Lift slope of the finite (and swept) wing, 1/rad.
    float minCL;             // Negative stall.
    float stallDrop;         // CL lost per radian beyond the stall angle.
    float CD0;               // Clean parasite drag coefficient.
    float oswald;            // Oswald span efficiency e.
    float criticalMach;      // Start of compressibility drag rise.
    std::vector<FlapSetting> flaps;
    int takeoffFlap;         // Flap index selected at spawn.
    // Propulsion.
    Propulsion propulsion;
    float maxThrust;         // N: total static sea-level thrust.
    float enginePower;       // W: shaft power (propeller only).
    float propEfficiency;    // Propeller efficiency (propeller only).
    // Control law (load-factor demand, Airbus-like) and handling.
    float maxLoadFactor,minLoadFactor;           // Clean configuration, g.
    float maxLoadFactorFlaps,minLoadFactorFlaps; // Flaps extended, g.
    float alphaProtectionMargin;                 // rad below stall kept as limit.
    float alphaTimeConstant;                     // s: short-period response.
    float maxRollRate;                           // rad/s at full lateral command.
    float rollStability;                         // 1/s wings-level tendency (0 = holds bank).
    // Ground handling.
    float wheelbase;          // m: nose/tail wheel to main gear.
    float maxSteerLow,maxSteerHigh;   // rad: steering angle at taxi / takeoff speed.
    float steerFadeStart,steerFadeEnd;// m/s: blend between the two angles.
    float rotationPitchRate;  // rad/s: nose-up rate during the takeoff rotation.
    float rotationSpeed;      // m/s: elevator reaches full authority on the ground.
    float rollingFriction;    // Tyre rolling resistance coefficient.
    float brakeFriction;      // Effective braking friction coefficient.

    float AspectRatio() const {return wingSpan*wingSpan/wingArea;}
    float InducedDragFactor() const;  // k = 1/(pi*e*AR)
};

// "a320" or "light" (AircraftCatalog ids). Throws std::out_of_range otherwise.
const FlightProfile& FlightProfileFor(const std::string& aircraftId);
#endif
