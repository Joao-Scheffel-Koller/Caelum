#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP


// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>


class Aircraft{
    private:
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 acceleration;
        glm::vec3 forward;
        glm::vec3 up;
        glm::vec3 right;


        float mass;
        float pitch;
        float yaw;
        float roll;
        float throttle;
        float throttle_increase_rate;
        float max_throttle;
        float max_scalar_velocity;

        float lift_factor;
        float drag_factor;
        
    public:
        Aircraft();
        
        //setters
        void set_throttle(float value);
        void set_pitch(float value);
        void set_yaw(float value);
        void set_roll(float value);
        void set_mass(float value);
        void set_drag_factor(float value);
        void set_lift_factor(float value);

        void set_position(glm::vec3 p);
        void set_velocity(glm::vec3 v);
        void set_acceleration(glm::vec3 a);
        void set_forward(glm::vec3 v);
        void set_right(glm::vec3 v);
        void set_up(glm::vec3 v);

        //getters
        float get_pitch();
        float get_throttle();
        float get_yaw();
        float get_roll();
        float get_throttle_increase_rate();
        float get_max_throttle();
        float get_max_scalar_velocity();
        float get_mass();
        float get_lift_factor();
        float get_drag_factor();

        glm::vec3 get_position();
        glm::vec3 get_velocity();
        glm::vec3 get_acceleration();
        glm::vec3 get_forward();
        glm::vec3 get_right();
        glm::vec3 get_up();  

};
#endif