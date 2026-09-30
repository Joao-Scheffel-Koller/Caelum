#include "Aircraft.hpp"

class FlightModel{

    private:
        float gravity;

    public:
        FlightModel();
        void update(Aircraft* a, float dt);
        void increase_throttle(Aircraft* a);
        void decrease_throttle(Aircraft* a);
};