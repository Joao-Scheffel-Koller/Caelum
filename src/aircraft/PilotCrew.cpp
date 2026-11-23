#include "aircraft/PilotCrew.hpp"
#include <glm/matrix.hpp>

// FONTE: codigo base do professor, include/matrices.h (definido em main.cpp).
// Forward declarations avoid duplicating the header's non-inline definitions.
glm::mat4 Matrix_Translate(float tx,float ty,float tz);
glm::mat4 Matrix_Scale(float sx,float sy,float sz);
glm::mat4 Matrix_Rotate_Y(float angle);

namespace pilot {
glm::mat4 SeatMatrix(const glm::vec3& seat,float scale) {
    const float quarterTurn=1.5707963f;   // R_y(90 deg): +Z -> +X, +X -> -Z.
    return Matrix_Translate(seat.x,seat.y,seat.z)*
           Matrix_Rotate_Y(quarterTurn)*
           Matrix_Scale(scale,scale,scale)*
           Matrix_Translate(-SeatAnchor.x,-SeatAnchor.y,-SeatAnchor.z);
}

bool CameraInsidePilot(const glm::mat4& aircraftModel,const glm::vec3& seat,float scale,
                       const glm::vec3& camera) {
    const glm::mat4 toPilot=glm::inverse(aircraftModel*SeatMatrix(seat,scale));
    const glm::vec3 p=glm::vec3(toPilot*glm::vec4(camera,1.0f));
    return p.x>=BoundsMin.x&&p.x<=BoundsMax.x&&p.y>=BoundsMin.y&&p.y<=BoundsMax.y&&
           p.z>=BoundsMin.z&&p.z<=BoundsMax.z;
}
} // namespace pilot
