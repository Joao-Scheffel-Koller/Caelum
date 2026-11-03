#ifndef CAELUM_AIRCRAFT_LIGHTS_INSPECTION_HPP
#define CAELUM_AIRCRAFT_LIGHTS_INSPECTION_HPP
#include <glad/glad.h>
#include <string>
struct GLFWwindow;

// Developer-only GPU checks and repeatable photographs of the two real assets.
// Requires a current context and the loaded scene shader; throws on failure.
void InspectAircraftLights(GLFWwindow* window,GLuint sceneProgram,const std::string& prefix);
#endif
