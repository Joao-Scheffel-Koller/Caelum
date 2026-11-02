#ifndef CAELUM_FLIGHT_HUD_HPP
#define CAELUM_FLIGHT_HUD_HPP
#include "aircraft/FlightTelemetry.hpp"
#include "ui/HudLayout.hpp"
#include "ui/HudNeedleMotion.hpp"
#include <glad/glad.h>
#include <string>
#include <vector>
struct GLFWwindow;

// AI-assisted 2D instrument renderer. Reads telemetry; never changes the aircraft.
class FlightHud {
public:
    FlightHud();
    ~FlightHud();
    FlightHud(const FlightHud&)=delete;
    FlightHud& operator=(const FlightHud&)=delete;
    // Update visual motion every frame, even when hidden; numbers remain real.
    void Update(const FlightTelemetry& telemetry,double elapsedSeconds);
    void Draw(GLFWwindow* window,const FlightTelemetry& telemetry);
private:
    struct Color {float r,g,b,a;};
    struct Vertex {float x,y,r,g,b,a,u,v,mode;};
    struct Label {float x,y,scale;std::string text;};
    GLuint program=0,vao=0,vbo=0;
    GLint resolutionUniform=-1,attitudeUniform=-1;
    std::vector<Vertex> vertices;
    std::vector<Label> labels;
    HudNeedleMotion needleMotion;
    FlightTelemetry needleReadings;
    void Triangle(float ax,float ay,float bx,float by,float cx,float cy,Color color);
    void Line(float ax,float ay,float bx,float by,float thickness,Color color);
    void Disc(float x,float y,float radius,Color color);
    void Ring(float x,float y,float radius,float thickness,Color color);
    void Text(float x,float baseline,const std::string& text,float scale);
    void Needle(const HudDial& dial,double angle,bool valid);
    void Gauge(const HudDial& dial,const FlightTelemetry& telemetry,float scale);
};
#endif
