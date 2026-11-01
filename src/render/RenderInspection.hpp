#ifndef CAELUM_RENDER_INSPECTION_HPP
#define CAELUM_RENDER_INSPECTION_HPP
#include "camera/CameraController.hpp"
#include <string>
struct GLFWwindow;

// Developer-only repeatable views: capture the same locations before/after fixes.
class RenderInspection {
public:
    explicit RenderInspection(const std::string& prefix):prefix(prefix) {}
    CameraPose Camera(const CameraFrame& aircraft) const;
    bool Cockpit() const;
    bool Capture(GLFWwindow* window);
private:
    std::string prefix;
    int frame=0;
};
#endif
