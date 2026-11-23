#include "aircraft/AircraftCatalog.hpp"

const std::vector<AircraftSpec>& AircraftCatalog() {
    static const std::vector<AircraftSpec> catalog={
        {"a320","Airbus A320-200","Jato comercial | Cabine e passageiros",
         "../../data/aircraft/aircraft.obj",{7.90f,.95f,-.65f},false},
        {"light","Monomotor leve","Alta asa | Motor a helice | Modelo estilizado",
         "../../data/light_aircraft/aircraft.obj",{2.44f,2.40f,-.26f},true,
         // OBJ survey: cushions top Y=1.695, back rests' front face X=2.00,
         // left seat (cockpit camera) Z<0, right seat Z>0. Z and scale chosen by
         // casting rays from the centre line to every pilot vertex: at the seat
         // centres (Z=-+0.30) and scale 0.8 the outer arms crossed the cabin
         // skin by up to 9 cm. Moved 6 cm (left) and 4 cm (right) inwards, as
         // the model's outer arm reaches further on the left, and scale 0.72
         // (this stylized cabin is smaller than a real one): nothing crosses
         // the skin and the pilots stay 5.8 cm apart (docs/pilot.md).
         pilot::Crew{{{2.00f,1.695f,-.24f},{2.00f,1.695f,.26f}},.72f}}
    };
    return catalog;
}
