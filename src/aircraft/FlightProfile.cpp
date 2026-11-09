#include "aircraft/FlightProfile.hpp"
#include <stdexcept>

namespace {
const float Degree=0.01745329252f;
const float Knot=0.514444f;

FlightProfile A320() {
    FlightProfile p;
    p.id="a320";
    // FONTE: Airbus, "A320 Aircraft Characteristics - Airport and Maintenance
    // Planning": MTOW 78 000 kg, OEW ~42 600 kg, wing area 122.6 m^2, span
    // 34.10 m (without sharklets, matching the 34.36 m mesh).
    // 60 t: typical short-haul takeoff mass (Santos Dumont operations are
    // weight-limited by its 1 323 m runway).
    p.mass=60000.0f;
    p.wingArea=122.6f;
    p.wingSpan=34.10f;
    // Lift slope: Helmbold/DATCOM finite wing 2*pi*AR/(2+sqrt(AR^2+4)) ~= 5.1/rad,
    // times cos(25 deg) quarter-chord sweep ~= 4.6; 4.8 is used.
    p.CL0=0.25f;
    p.CLalpha=4.8f;
    p.minCL=-0.8f;
    p.stallDrop=2.0f;
    // Clean parasite drag: with e=0.80 and AR=9.5, max L/D = 1/(2 sqrt(CD0 k))
    // ~17, the published A320 value (16-18).
    p.CD0=0.020f;
    p.oswald=0.80f;
    // Lock: drag divergence (dCD/dM=0.1) at Mcrit+0.108; A320 Mdd ~0.80.
    p.criticalMach=0.70f;
    // CLmax values reproduce published 1-g stall speeds at 60-64 t: clean ~150 kt,
    // CONF 1+F ~125 kt, CONF FULL ~105 kt (Vref ~130 kt).
    p.flaps={
        {"0",   0.00f,1.50f,0.000f},
        {"1+F", 0.40f,2.10f,0.010f},
        {"2",   0.55f,2.35f,0.020f},
        {"3",   0.70f,2.55f,0.030f},
        {"FULL",0.90f,2.85f,0.050f},
    };
    p.takeoffFlap=1;
    // FONTE: CFM56-5B4 (A320-214) ~120 kN (27 000 lbf) static thrust per engine.
    p.propulsion=Propulsion::Turbofan;
    p.maxThrust=2*120100.0f;
    p.enginePower=0;p.propEfficiency=0;
    // FONTE: A320 FCOM, flight control normal law: +2.5/-1.0 g clean,
    // +2.0/0 g with slats/flaps extended; alpha protection near the stall.
    p.maxLoadFactor=2.5f;p.minLoadFactor=-1.0f;
    p.maxLoadFactorFlaps=2.0f;p.minLoadFactorFlaps=0.0f;
    p.alphaProtectionMargin=2.0f*Degree;
    p.alphaTimeConstant=0.6f;
    p.maxRollRate=15.0f*Degree;   // Normal law: full sidestick = 15 deg/s.
    p.rollStability=0.0f;         // Normal law: neutral sidestick holds the bank.
    // FONTE: Airbus AC document: wheelbase 12.64 m; tiller up to 75 deg,
    // pedals +-6 deg at speed.
    p.wheelbase=12.64f;
    p.maxSteerLow=70.0f*Degree;p.maxSteerHigh=6.0f*Degree;
    p.steerFadeStart=3.0f;p.steerFadeEnd=30.0f;
    p.rotationPitchRate=3.0f*Degree; // Standard rotation rate ~3 deg/s.
    p.rotationSpeed=135.0f*Knot;     // ~VR at this mass.
    p.rollingFriction=0.015f;
    p.brakeFriction=0.40f;
    return p;
}

FlightProfile Light() {
    FlightProfile p;
    p.id="light";
    // The mesh is a stylized high-wing single engine (9 m long, 11 m span), so it
    // uses Cessna 172S-class values: MTOW 1 157 kg, S 16.2 m^2, span 11.0 m,
    // 180 hp, Vs clean 48 kt, Vs flaps 40 kt, Vne 163 kt.
    p.mass=1000.0f;
    p.wingArea=16.2f;
    p.wingSpan=11.0f;
    p.CL0=0.30f;
    p.CLalpha=4.6f;
    p.minCL=-0.8f;
    p.stallDrop=2.0f;
    p.CD0=0.031f;       // Fixed gear and strut.
    p.oswald=0.75f;
    p.criticalMach=0.60f;
    p.flaps={
        {"0", 0.00f,1.60f,0.000f},
        {"10",0.25f,1.80f,0.010f},
        {"20",0.45f,2.00f,0.025f},
        {"30",0.60f,2.10f,0.040f},
    };
    p.takeoffFlap=1;
    p.propulsion=Propulsion::Propeller;
    p.maxThrust=2900.0f;      // Static thrust of a 180 hp fixed-pitch propeller.
    p.enginePower=134000.0f;  // 180 hp.
    p.propEfficiency=0.80f;
    // Normal category limits: +3.8/-1.52 g, +3.0/0 g with flaps.
    p.maxLoadFactor=3.8f;p.minLoadFactor=-1.52f;
    p.maxLoadFactorFlaps=3.0f;p.minLoadFactorFlaps=0.0f;
    p.alphaProtectionMargin=1.0f*Degree;
    p.alphaTimeConstant=0.3f;
    p.maxRollRate=45.0f*Degree;
    p.rollStability=0.3f;     // Dihedral: slowly returns to wings level.
    // Wheel survey of the converted mesh (data/light_aircraft/metadata.json):
    // main gear X=3.08 m, tail wheel X=-3.06 m.
    p.wheelbase=6.1f;
    p.maxSteerLow=30.0f*Degree;p.maxSteerHigh=8.0f*Degree;
    p.steerFadeStart=3.0f;p.steerFadeEnd=20.0f;
    p.rotationPitchRate=8.0f*Degree;
    p.rotationSpeed=50.0f*Knot;
    p.rollingFriction=0.02f;
    p.brakeFriction=0.35f;
    return p;
}
} // namespace

float FlightProfile::InducedDragFactor() const {
    return 1.0f/(3.14159265f*oswald*AspectRatio());
}

const FlightProfile& FlightProfileFor(const std::string& id) {
    static const FlightProfile a320=A320(),light=Light();
    if(id==a320.id)return a320;
    if(id==light.id)return light;
    throw std::out_of_range("Unknown flight profile: "+id);
}
