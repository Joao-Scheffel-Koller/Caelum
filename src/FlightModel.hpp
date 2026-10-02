#include "Aircraft.hpp"

class FlightModel{

    private:
        float gravity;
        void update_orientation(Aircraft* aircraft, float mouse_delta_x, float mouse_delta_y);

    public:
        FlightModel();
        void update(Aircraft* a, float dt, float mouse_delta_x, float mouse_delta_y);
        void increase_throttle(Aircraft* a);
        void decrease_throttle(Aircraft* a);
};