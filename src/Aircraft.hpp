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
        void set_throttle(float value);
        float get_pitch();

};
#endif