#ifndef CAELUM_MISSION_MARKER_HPP
#define CAELUM_MISSION_MARKER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>

struct GLFWwindow;

// AI-assisted navigation billboard. Its anchor is a fixed world location above
// the destination airport; visibility belongs to the caller, not the mission.
class MissionMarker {
public:
    MissionMarker();
    ~MissionMarker();
    MissionMarker(const MissionMarker&)=delete;
    MissionMarker& operator=(const MissionMarker&)=delete;
    void Draw(GLFWwindow* window,const glm::mat4& view,const glm::mat4& projection,
              const glm::vec3& camera) const;
private:
    glm::dvec3 anchor{0};
    GLuint program=0,vao=0,vbo=0;
    GLint anchorUniform=-1,sizeUniform=-1;
};
#endif
