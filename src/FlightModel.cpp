#include "FlightModel.hpp"
#include "Aircraft.hpp"
#include <stdio.h>

/*void FlightModel::update(Aircraft* a, float dt)
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
} */

FlightModel :: FlightModel(){
    gravity = 1.0f;
}

void FlightModel::update( Aircraft* aircraft, float dt)
{
    printf("dt = %f\n", dt);
    // 1. Propulsão
    float thrust = aircraft->get_throttle() * aircraft->get_max_throttle();
    glm::vec3 thrustForce = aircraft->get_forward() * thrust;


    // 2. Gravidade
    glm::vec3 gravityForce = glm::vec3{
        0.0f,
        gravity * aircraft->get_mass(),
        0.0f
    };


    // 3. Velocidade atual
    glm::vec3 velocity = aircraft->get_velocity();
    float speed = velocity.length();


    // 4. Sustentação
    float lift = aircraft->get_lift_factor() * speed * speed;
    glm::vec3 liftForce = aircraft->get_up() * lift;


    // 5. Arrasto
    glm::vec3 dragForce{0.0f, 0.0f, 0.0f};

    if (speed > 0.0f)
    {
        dragForce =
            - glm::normalize(velocity)
            * aircraft->get_drag_factor()
            * speed
            * speed;
    }


    // 6. Força resultante
    glm::vec3 totalForce =
        thrustForce +
        gravityForce +
        liftForce +
        dragForce;


    // 7. Segunda lei de Newton
    glm::vec3 acceleration =
        totalForce / aircraft->get_mass();


    // 8. Atualiza velocidade
    velocity += acceleration * dt;
    aircraft->set_velocity(velocity);


    // 9. Atualiza posição
    glm::vec3 position = aircraft->get_position();
    position += velocity * dt;
    aircraft->set_position(position);
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