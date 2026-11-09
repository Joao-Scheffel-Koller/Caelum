#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP


// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "aircraft/FlightProfile.hpp"


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

        // AI-assisted: physical parameters (mass, wing, engines...) of the
        // selected aircraft, and the aerodynamic state used by FlightModel.
        const FlightProfile* profile;
        int flap_index;
        float alpha;      // Angle of attack, rad.
        bool brakes;
        bool path_hold;        // Neutral sidestick: holding path_reference.
        float path_reference;  // Flight path angle to hold, rad.
        
    public:
        Aircraft();
        
        //setters
        void set_throttle(float value);
        void set_pitch(float value);
        void set_yaw(float value);
        void set_roll(float value);
        void set_profile(const FlightProfile& p);
        void set_flap_index(int value);
        void set_alpha(float value);
        void set_brakes(bool value);
        void set_path_hold(bool hold, float reference);

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
        float get_mass();
        const FlightProfile& get_profile();
        int get_flap_index();
        const FlapSetting& get_flaps();
        float get_alpha();
        bool get_brakes();
        bool get_path_hold();
        float get_path_reference();

        glm::vec3 get_position();
        glm::vec3 get_velocity();
        glm::vec3 get_acceleration();
        glm::vec3 get_forward();
        glm::vec3 get_right();
        glm::vec3 get_up();  

};
#endif