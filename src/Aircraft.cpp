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

        void Aircraft::set_throttle(float value){throttle = value;}
        void Aircraft::set_pitch(float value){pitch = value;}
        void Aircraft::set_yaw(float value){yaw = value;}
        void Aircraft::set_roll(float value){roll = value;}

        void Aircraft::set_position(glm::vec3 p){position = p;}
        void Aircraft::set_velocity(glm::vec3 v){velocity = v;}
        void Aircraft::set_acceleration(glm::vec3 a){acceleration = a;}

        //getters
        float Aircraft::get_pitch(){return pitch;}
        float Aircraft::get_throttle(){return throttle;}
        float Aircraft::get_yaw(){return yaw;}
        float Aircraft::get_roll(){return roll;}

        glm::vec3 Aircraft::get_position(){return position;}
        glm::vec3 Aircraft::get_velocity(){return velocity;}
        glm::vec3 Aircraft::get_acceleration(){return acceleration;}
        
        


        