#include "Aircraft.hpp" 
 
Aircraft :: Aircraft()
    {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = glm::vec3(0.0f, 0.0f, 0.0f);

        throttle = 0.0f;
        roll = 0.0f;
        pitch = 0.0f;
        yaw = 0.0f;
    }


        //placeholders for getters and setters

void Aircraft::set_throttle(float value)
    {
        throttle = value;
    }

float Aircraft::get_pitch()
{
    return pitch;
}