#ifndef CAELUM_COLLISION_DEBUG_HPP
#define CAELUM_COLLISION_DEBUG_HPP
#include "collision/AircraftContacts.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// AI-assisted wire overlay of the exact sphere probes used by the resolver.
class CollisionDebug {
public:
    CollisionDebug();
    ~CollisionDebug();
    CollisionDebug(const CollisionDebug&)=delete;
    CollisionDebug& operator=(const CollisionDebug&)=delete;
    void Draw(const std::vector<collision::ProbeSphere>& probes,const glm::mat4& view,
              const glm::mat4& projection,const glm::vec3& camera);
private:
    GLuint program=0,vao=0,vbo=0;
    GLint viewUniform=-1,projectionUniform=-1;
};
#endif
