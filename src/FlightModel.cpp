#include "FlightModel.hpp"
#include "Aircraft.hpp"
#include <stdio.h>
#include "mathops.hpp"


FlightModel :: FlightModel(){
    //para testes, gravity = 0,mas depois mudar para gravity = -1.0f
    gravity = -1.0f;
}

void FlightModel:: update_orientation(Aircraft* aircraft, float mouse_delta_x, float mouse_delta_y)
{
    float roll = aircraft->get_roll();
    float yaw = aircraft->get_yaw();
    float pitch = aircraft->get_pitch();
    aircraft->set_roll(roll + mouse_delta_x * 0.01f);
    aircraft->set_yaw(0.0f);
    aircraft->set_pitch(pitch - mouse_delta_y * 0.01f);

    glm::vec4 new_forward, new_up, new_right, old_forward, old_up, old_right;

    old_forward = glm::vec4(aircraft->get_forward(), 0.0f);
    old_up = glm::vec4(aircraft->get_up(), 0.0f);
    old_right = glm::vec4(aircraft->get_right(), 0.0f);

    new_forward = old_forward * Matrix_Rotate_X(roll) * Matrix_Rotate_Y(yaw) * Matrix_Rotate_Z(pitch);
    new_up = old_up * Matrix_Rotate_X(roll) * Matrix_Rotate_Y(yaw) * Matrix_Rotate_Z(pitch);
    new_right = old_right * Matrix_Rotate_X(roll) * Matrix_Rotate_Y(yaw) * Matrix_Rotate_Z(pitch);

    aircraft->set_forward(glm::vec3(new_forward));
    aircraft->set_up(glm::vec3(new_up));
    aircraft->set_right(glm::vec3(new_right));

}

void FlightModel::update( Aircraft* aircraft, float dt, float mouse_delta_x, float mouse_delta_y)
{

    update_orientation(aircraft, mouse_delta_x, mouse_delta_y);

    // 1. Propulsão
    float thrust = aircraft->get_throttle() * aircraft->get_max_thrust();
    glm::vec3 thrustForce = aircraft->get_forward() * thrust;


    // 2. Gravidade
    glm::vec3 gravityForce = glm::vec3{
        0.0f,
        gravity * aircraft->get_mass(),
        0.0f
    };

    // 3. Velocidade atual
    glm::vec3 velocity = aircraft->get_velocity();
    //.length() is not used because it was always rounding the number up to 3.0f
    float speed = std::sqrt(
    velocity.x * velocity.x +
    velocity.y * velocity.y +
    velocity.z * velocity.z
    );

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
    //ajusta colisão com o solo
    if (position.y < 0.0f){
        position.y = 0.0f;
    }
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