#ifndef CAELUM_FRONTEND_INSPECTION_HPP
#define CAELUM_FRONTEND_INSPECTION_HPP
#include "ui/FrontendModel.hpp"
#include "ui/FrontendScreen.hpp"
#include <string>
struct GLFWwindow;

// AI-assisted developer-only fixtures: never loads/moves an aircraft or map.
class FrontendInspection {
public:
    explicit FrontendInspection(const std::string& prefix):prefix(prefix) {}
    void Prepare(GLFWwindow*,frontend::FrontendModel&);
    void Draw(GLFWwindow*,frontend::FrontendScreen&,const frontend::FrontendModel&,double);
    bool Capture(GLFWwindow*);
private:
    std::string prefix;
    int frame=0;
};
// Read back before swapping buffers. Also used to inspect actual loading work.
void CaptureFrontendFrame(GLFWwindow*,const std::string& path);
bool SelectFrontendAircraft(frontend::FrontendModel&,std::size_t index,int width,int height);
#endif
