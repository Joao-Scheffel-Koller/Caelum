#ifndef CAELUM_AIRCRAFT_CATALOG_HPP
#define CAELUM_AIRCRAFT_CATALOG_HPP
#include <glm/vec3.hpp>
#include <string>
#include <vector>

// AI-assisted asset configuration shared by selection and scene loading.
// These names describe the actual meshes; selecting one never replaces physics
// with menu telemetry or an unrelated aircraft model.
struct AircraftSpec {
    std::string id,name,description,modelPath;
    glm::vec3 cockpitEye;
    bool light=false;
    AircraftSpec(const std::string& id,const std::string& name,const std::string& description,
                 const std::string& path,const glm::vec3& eye,bool light)
        :id(id),name(name),description(description),modelPath(path),cockpitEye(eye),light(light) {}
};
const std::vector<AircraftSpec>& AircraftCatalog();
#endif
