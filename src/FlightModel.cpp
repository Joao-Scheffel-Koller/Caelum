#include "FlightModel.hpp"
#include "Aircraft.hpp"
#include <stdio.h>

void FlightModel::update(Aircraft* a, float dt)
{
    //Vec3 acceleration = calculateAcceleration();
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    a->set_acceleration(acceleration);

    glm::vec3 velocity = a->get_velocity();
    velocity += a->get_throttle() * dt;
    a->set_velocity(velocity);

    glm::vec3 position = a->get_position();
    position += velocity * dt;
    a->set_position(position);
}

void FlightModel :: increase_throttle(Aircraft* a)
{
    float new_throttle = std::fmin(a->get_throttle() + a->get_throttle_increase_rate(), a->get_max_throttle());
    a->set_throttle(new_throttle);
    printf("throttle: %f\n", new_throttle);
}
void FlightModel ::  decrease_throttle(Aircraft* a)
{
    float new_throttle = std::fmax(a->get_throttle() - a->get_throttle_increase_rate(), 0.0f);
    a->set_throttle(new_throttle); 
    printf("throttle: %f\n", new_throttle);
}