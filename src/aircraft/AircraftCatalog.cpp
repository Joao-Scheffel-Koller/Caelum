#include "aircraft/AircraftCatalog.hpp"

const std::vector<AircraftSpec>& AircraftCatalog() {
    static const std::vector<AircraftSpec> catalog={
        {"a320","Airbus A320-200","Jato comercial | Cabine e passageiros",
         "../../data/aircraft/aircraft.obj",{7.90f,.95f,-.65f},false},
        {"light","Monomotor leve","Alta asa | Motor a helice | Modelo estilizado",
         "../../data/light_aircraft/aircraft.obj",{2.44f,2.40f,-.26f},true}
    };
    return catalog;
}
