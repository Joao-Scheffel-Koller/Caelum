#include "aircraft/AircraftCatalog.hpp"

const std::vector<AircraftSpec>& AircraftCatalog() {
    static const std::vector<AircraftSpec> catalog={
        {"a320","Airbus A320-200","Jato comercial | Cabine e passageiros",
         "../../data/aircraft/aircraft.obj",{7.90f,.95f,-.65f},false},
        {"light","Monomotor leve","Alta asa | Motor a helice | Modelo estilizado",
         "../../data/light_aircraft/aircraft.obj",{2.44f,2.40f,-.26f},true,
         // OBJ survey: cushions top Y=1.695, back rests' front face X=2.00,
         // seat centres Z=-0.30 (left, cockpit camera) and +0.30 (right).
         // Scale 0.8: this stylized cabin puts the camera eye 0.70 m above the
         // seat; the pilot model's eyes are 0.89 m above his hips.
         pilot::Crew{{{2.00f,1.695f,-.30f},{2.00f,1.695f,.30f}},.80f}}
    };
    return catalog;
}
