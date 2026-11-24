#include "aircraft/AircraftCatalog.hpp"

const std::vector<AircraftSpec>& AircraftCatalog() {
    static const std::vector<AircraftSpec> catalog={
        {"a320","Airbus A320-200","Jato comercial | Cabine e passageiros",
         "../../data/aircraft/aircraft.obj",{7.90f,.95f,-.65f},false,
         // OBJ survey (a_chair* objects, real-size model in metres, scale 1):
         // seat centres Z=-+0.69 (left = captain = cockpit camera); the
         // reclined back rest's front face is at X=7.73 at shoulder height;
         // cushion top Y=0.147, raised to 0.16 so the shoes rest on the
         // cockpit floor (Y=-0.258) instead of sinking into it. The first
         // officer sits 1 cm outwards (Z=+0.70): the pilot model is asymmetric
         // and his right knee touched the knee pad under the panel by 9 mm
         // (tests/PilotFitTest.cpp, docs/pilot.md).
         pilot::Crew{{{7.73f,.16f,-.69f},{7.73f,.16f,.70f}},1.0f}},
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
         pilot::Crew{{{2.00f,1.695f,-.24f},{2.00f,1.695f,.26f}},.72f},
         // OBJ survey: the 3 blades are the objects "wing", "wing.001",
         // "wing.002" (Propeller_* materials) and the spinner is "wingRoot".
         // data/light_aircraft/tools/convert_asset.py stretched the source by
         // 9/9.0396 (length), 3/2.6481 (height) and 11/8 (span). Undoing that,
         // the 3 blades coincide after a 120 degree turn (error < 0.1 mm)
         // about a shaft tilted 12.896 degrees nose-up, through this hub.
         // Blade scale: geometric mean of height and span, giving 1.035 m.
         // 2700 rpm: maximum of a 180 hp Lycoming O-360 (docs/propeller.md).
         propeller::Spec{{"wing","wing.001","wing.002"},{"wingRoot"},{4.2646f,1.9345f,0.0f},
                         {.99562f,1.13289f,1.375f},.225078f,1.2481f,1.035f,2700.0f}}
    };
    return catalog;
}
