#include "FlightModel.hpp"

FlightModel::FlightModel(Aircraft aircraft)
    : aircraft(aircraft)
{
}
void set_aircraft(){}
void remove_aircraft(){}
void FlightModel::update(float dt)
{
    //Vec3 acceleration = calculateAcceleration();
    glm::vec3 acceleration = glm::vec3(1.0f, 1.0f, 1.0f);
    aircraft.set_acceleration(acceleration);

    glm::vec3 velocity = aircraft.get_velocity();
    velocity += acceleration * dt;
    aircraft.set_velocity(velocity);

    glm::vec3 position = aircraft.get_position();
    position += velocity * dt;
    aircraft.set_position(position);
}