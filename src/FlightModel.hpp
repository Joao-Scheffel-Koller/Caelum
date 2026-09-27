#include "Aircraft.hpp"

class FlightModel{

    public:
        void update(Aircraft* a, float dt);
        void increase_throttle(Aircraft* a);
        void decrease_throttle(Aircraft* a);
};