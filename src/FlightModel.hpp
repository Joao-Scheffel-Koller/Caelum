#include "Aircraft.hpp"

class FlightModel{
    private:
        Aircraft aircraft;
    public:
        FlightModel(Aircraft aircraft);
        void set_aircraft();
        void remove_aircraft();
        void update(float dt);
};