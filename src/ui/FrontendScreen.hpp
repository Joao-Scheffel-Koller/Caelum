#ifndef CAELUM_FRONTEND_SCREEN_HPP
#define CAELUM_FRONTEND_SCREEN_HPP
#include "ui/FrontendModel.hpp"
#include <glad/glad.h>
#include <string>
#include <vector>
struct GLFWwindow;
namespace frontend {
// AI-assisted procedural frontend. Draws only; model owns choices and input.
class FrontendScreen {
public:
    FrontendScreen();
    ~FrontendScreen();
    FrontendScreen(const FrontendScreen&)=delete;
    FrontendScreen& operator=(const FrontendScreen&)=delete;
    void Draw(GLFWwindow* window,const FrontendModel& model,double elapsedSeconds);
    void DrawLoading(GLFWwindow* window,double fraction,const std::string& stage,
                     const std::string& message,double elapsedSeconds);
private:
    struct Color {float r,g,b,a;};
    struct Vertex {float x,y,r,g,b,a;};
    struct Label {float x,y,scale,maxWidth;std::string text;};
    GLuint program=0,vao=0,vbo=0;
    GLint resolutionUniform=-1;
    std::vector<Vertex> vertices;
    std::vector<Label> labels;
    int width=0,height=0;
    void Triangle(float ax,float ay,float bx,float by,float cx,float cy,Color color);
    void Rectangle(const Rect& rect,Color color);
    void Line(float ax,float ay,float bx,float by,float thickness,Color color);
    void Text(float x,float y,const std::string& text,float scale,float maxWidth);
    void Plane(float x,float y,float size,bool light,Color color);
    void Background(double elapsedSeconds);
    void Flush(GLFWwindow* window);
};
} // namespace frontend
#endif
