#include "aircraft/Aircraft.hpp"
 
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
        // Throttle is a fraction of the maximum thrust (0..1).
        max_throttle = 1.0f;
        // Realistic parameters live in FlightProfile (docs/physics.md).
        profile = &FlightProfileFor("a320");
        mass = profile->mass;
        flap_index = profile->takeoffFlap;
        alpha = 0.0f;
        brakes = false;
        path_hold = false;
        path_reference = 0.0f;
    }


        //placeholders for getters and setters

        //setters

        void Aircraft::set_throttle(float value){throttle = value;}
        void Aircraft::set_pitch(float value){pitch = value;}
        void Aircraft::set_yaw(float value){yaw = value;}
        void Aircraft::set_roll(float value){roll = value;}
        void Aircraft::set_profile(const FlightProfile& p){profile = &p; mass = p.mass; flap_index = p.takeoffFlap;}
        void Aircraft::set_flap_index(int value){
            const int last = static_cast<int>(profile->flaps.size()) - 1;
            flap_index = value < 0 ? 0 : (value > last ? last : value);
        }
        void Aircraft::set_alpha(float value){alpha = value;}
        void Aircraft::set_brakes(bool value){brakes = value;}
        void Aircraft::set_path_hold(bool hold, float reference){path_hold = hold; path_reference = reference;}

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
        float Aircraft::get_mass(){return mass;}
        const FlightProfile& Aircraft::get_profile(){return *profile;}
        int Aircraft::get_flap_index(){return flap_index;}
        const FlapSetting& Aircraft::get_flaps(){return profile->flaps[flap_index];}
        float Aircraft::get_alpha(){return alpha;}
        bool Aircraft::get_brakes(){return brakes;}
        bool Aircraft::get_path_hold(){return path_hold;}
        float Aircraft::get_path_reference(){return path_reference;}

        glm::vec3 Aircraft::get_position(){return position;}
        glm::vec3 Aircraft::get_velocity(){return velocity;}
        glm::vec3 Aircraft::get_acceleration(){return acceleration;}
        glm::vec3 Aircraft::get_forward(){return forward;}
        glm::vec3 Aircraft::get_right(){return right;}
        glm::vec3 Aircraft::get_up(){return up;}
