#ifndef CAELUM_PILOT_CREW_HPP
#define CAELUM_PILOT_CREW_HPP
// AI-assisted: pilots seated in the aircraft (docs/pilot.md). One pilot mesh
// (data/pilot/pilot.obj) is drawn once per seat, each with its own Model
// matrix: aircraft Model * seat matrix. No OpenGL here, so it is testable.
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace pilot {
// Measured in data/pilot/pilot.obj (metres, pilot faces +Z, up +Y); see
// data/pilot/PROVENANCE.md. The anchor is where the pilot touches the seat:
// bottom of the hips (Y) in the plane of his back (Z).
const glm::vec3 SeatAnchor(0.0f,0.40f,-0.17f);
// Bounding box of the seated pilot (arms and knees included), 1 cm margin.
const glm::vec3 BoundsMin(-0.30f,-0.04f,-0.18f);
const glm::vec3 BoundsMax(0.40f,1.42f,0.59f);

// Seats of one aircraft, in the aircraft OBJ's coordinates (nose +X, up +Y):
// each point is the top of a seat cushion at the front face of its back rest.
struct Crew {
    std::vector<glm::vec3> seats;
    float scale=1.0f;              // Pilot size relative to the model's metres.
    Crew() {}
    Crew(const std::vector<glm::vec3>& seats,float scale):seats(seats),scale(scale) {}
};

// Pilot OBJ -> aircraft OBJ coordinates for one seat:
// T(seat) * R_y(90 deg) * S(scale) * T(-SeatAnchor). R_y(90 deg) turns the
// pilot's +Z (front) to the aircraft's +X (nose).
glm::mat4 SeatMatrix(const glm::vec3& seat,float scale);

// True when the camera is inside this pilot's body (his bounding box): that
// pilot is not drawn (cockpit view from his seat, or a free camera flown into
// him). The camera is taken to the pilot's own coordinates by the inverse of
// his Model matrix, where the box is axis-aligned.
bool CameraInsidePilot(const glm::mat4& aircraftModel,const glm::vec3& seat,float scale,
                      const glm::vec3& camera);
} // namespace pilot
#endif
