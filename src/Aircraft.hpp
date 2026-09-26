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

        float pitch;
        float yaw;
        float roll;
        float throttle;
    public:
        Aircraft();
        
        //setters
        void set_throttle(float value);
        void set_pitch(float value);
        void set_yaw(float value);
        void set_roll(float value);

        void set_position(glm::vec3 p);
        void set_velocity(glm::vec3 v);

        //getters
        float get_pitch();
        float get_throttle();
        float get_yaw();
        float get_roll();

        glm::vec3 get_position();
        glm::vec3 get_velocity();        

};
#endif