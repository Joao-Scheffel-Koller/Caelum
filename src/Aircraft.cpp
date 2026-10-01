#include "Aircraft.hpp" 
 
Aircraft :: Aircraft()
    {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        acceleration = glm::vec3(0.0f);
        forward = glm::vec3(-1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        right = glm::vec3(0.0f, 0.0f, -1.0f);

        throttle = 0.0f;
        roll = 0.0f;
        pitch = 0.0f;
        yaw = 0.0f;
        throttle_increase_rate = 0.05f;
        max_throttle = 1.0f;
        max_scalar_velocity = 2.0f;
        mass = 40.0f;
        lift_factor = 0.5f;
        drag_factor = 0.3f;
        max_thrust = 80.0f;
    }


        //placeholders for getters and setters

        //setters

        void Aircraft::set_throttle(float value){throttle = value;}
        void Aircraft::set_pitch(float value){pitch = value;}
        void Aircraft::set_yaw(float value){yaw = value;}
        void Aircraft::set_roll(float value){roll = value;}
        void Aircraft::set_mass(float value){mass = value;}
        void Aircraft::set_lift_factor(float value){lift_factor = value;}
        void Aircraft::set_drag_factor(float value){drag_factor = value;}

        void Aircraft::set_position(glm::vec3 p){position = p;}
        void Aircraft::set_velocity(glm::vec3 v){velocity = v;}
        void Aircraft::set_acceleration(glm::vec3 a){acceleration = a;}
        void Aircraft::set_forward(glm::vec3 v){forward = v;}
        void Aircraft::set_right(glm::vec3 v){right = v;}
        void Aircraft::set_up(glm::vec3 v){up = v;}


        //getters
        float Aircraft::get_pitch(){return pitch;}
        float Aircraft::get_throttle(){return throttle;}
        float Aircraft::get_yaw(){return yaw;}
        float Aircraft::get_roll(){return roll;}
        float Aircraft::get_throttle_increase_rate(){return throttle_increase_rate;}
        float Aircraft::get_max_throttle(){return max_throttle;}
        float Aircraft::get_max_scalar_velocity(){return max_scalar_velocity;}
        float Aircraft::get_mass(){return mass;}
        float Aircraft::get_lift_factor(){return lift_factor;}
        float Aircraft::get_drag_factor(){return drag_factor;}
        float Aircraft::get_max_thrust(){return max_thrust;}

        glm::vec3 Aircraft::get_position(){return position;}
        glm::vec3 Aircraft::get_velocity(){return velocity;}
        glm::vec3 Aircraft::get_acceleration(){return acceleration;}
        glm::vec3 Aircraft::get_forward(){return forward;}
        glm::vec3 Aircraft::get_right(){return right;}
        glm::vec3 Aircraft::get_up(){return up;}
        
        


        