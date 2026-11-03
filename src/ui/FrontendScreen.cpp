#include "ui/FrontendScreen.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

// FONTE: shader compilation and font renderer supplied by the professor,
// src/main.cpp and src/ui/textrendering.cpp. All frontend artwork is procedural.
GLuint LoadShader_Vertex(const char*);
GLuint LoadShader_Fragment(const char*);
GLuint CreateGpuProgram(GLuint,GLuint);
void TextRendering_PrintString(GLFWwindow*,const std::string&,float,float,float);
float TextRendering_CharWidth(GLFWwindow*);

namespace frontend {
namespace {
void Enabled(GLenum capability,bool enabled) {if(enabled)glEnable(capability);else glDisable(capability);}
struct State {
    GLint program,vao,buffer,depthFunc,polygon[2],srcRgb,dstRgb,srcAlpha,dstAlpha,eqRgb,eqAlpha;
    GLboolean depthWrite;
    bool depth,blend,cull,coverage,scissor;
    State():depth(glIsEnabled(GL_DEPTH_TEST)),blend(glIsEnabled(GL_BLEND)),
        cull(glIsEnabled(GL_CULL_FACE)),coverage(glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE)),
        scissor(glIsEnabled(GL_SCISSOR_TEST)) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqAlpha);
    }
    ~State() {
        glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glDepthFunc(depthFunc);glDepthMask(depthWrite);glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
        glBlendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha);glBlendEquationSeparate(eqRgb,eqAlpha);
        Enabled(GL_DEPTH_TEST,depth);Enabled(GL_BLEND,blend);Enabled(GL_CULL_FACE,cull);
        Enabled(GL_SAMPLE_ALPHA_TO_COVERAGE,coverage);Enabled(GL_SCISSOR_TEST,scissor);
    }
};
std::string Fit(std::string text,std::size_t capacity) {
    if(text.size()<=capacity)return text;
    if(capacity<=3)return text.substr(0,capacity);
    auto end=capacity-3;const auto space=text.rfind(' ',end);
    if(space!=std::string::npos&&space>end/2)end=space;
    return text.substr(0,end)+"...";
}
}

FrontendScreen::FrontendScreen() {
    const GLuint vertex=LoadShader_Vertex("../../src/ui/shaders/frontend_vertex.glsl");
    const GLuint fragment=LoadShader_Fragment("../../src/ui/shaders/frontend_fragment.glsl");
    program=CreateGpuProgram(vertex,fragment);glDeleteShader(vertex);glDeleteShader(fragment);
    resolutionUniform=glGetUniformLocation(program,"resolution");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
    glBindVertexArray(0);glBindBuffer(GL_ARRAY_BUFFER,0);
    vertices.reserve(3000);labels.reserve(50);
}
FrontendScreen::~FrontendScreen() {
    glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
}
void FrontendScreen::Triangle(float ax,float ay,float bx,float by,float cx,float cy,Color c) {
    vertices.push_back({ax,ay,c.r,c.g,c.b,c.a});vertices.push_back({bx,by,c.r,c.g,c.b,c.a});
    vertices.push_back({cx,cy,c.r,c.g,c.b,c.a});
}
void FrontendScreen::Rectangle(const Rect& r,Color c) {
    Triangle(r.x,r.y,r.x+r.width,r.y,r.x+r.width,r.y+r.height,c);
    Triangle(r.x,r.y,r.x+r.width,r.y+r.height,r.x,r.y+r.height,c);
}
void FrontendScreen::Line(float ax,float ay,float bx,float by,float thickness,Color c) {
    const float dx=bx-ax,dy=by-ay,length=std::hypot(dx,dy);
    if(length<1e-6f)return;
    const float x=-dy/length*thickness*.5f,y=dx/length*thickness*.5f;
    Triangle(ax+x,ay+y,bx+x,by+y,bx-x,by-y,c);Triangle(ax+x,ay+y,bx-x,by-y,ax-x,ay-y,c);
}
void FrontendScreen::Text(float x,float y,const std::string& text,float scale,float maxWidth) {
    labels.push_back({x,y,scale,maxWidth,text});
}
void FrontendScreen::Plane(float x,float y,float size,bool light,Color color) {
    const float angle=-.24f,cs=std::cos(angle),sn=std::sin(angle);
    const auto triangle=[&](float ax,float ay,float bx,float by,float cx,float cy) {
        Triangle(x+size*(cs*ax-sn*ay),y+size*(sn*ax+cs*ay),
                 x+size*(cs*bx-sn*by),y+size*(sn*bx+cs*by),
                 x+size*(cs*cx-sn*cy),y+size*(sn*cx+cs*cy),color);
    };
    triangle(-.85f,-.055f,.76f,-.09f,.98f,0);triangle(-.85f,-.055f,.98f,0,-.85f,.055f);
    triangle(-.85f,.055f,.98f,0,.76f,.09f);
    for(float side:{-1.0f,1.0f}) {
        triangle(.20f,side*.06f,light?.28f:-.26f,side*.86f,light?-.06f:-.49f,side*.85f);
        triangle(.20f,side*.06f,light?-.06f:-.49f,side*.85f,-.16f,side*.06f);
        triangle(-.57f,side*.04f,-.84f,side*.35f,-.92f,side*.32f);
        triangle(-.57f,side*.04f,-.92f,side*.32f,-.86f,side*.04f);
        if(!light) {
            triangle(.10f,side*.34f,.38f,side*.34f,.38f,side*.42f);
            triangle(.10f,side*.34f,.38f,side*.42f,.10f,side*.42f);
        }
    }
    if(light) {
        triangle(.92f,-.30f,.96f,-.30f,.96f,.30f);triangle(.92f,-.30f,.96f,.30f,.92f,.30f);
    }
}
void FrontendScreen::Background(double elapsed) {
    const Color top={.018f,.035f,.065f,1},bottom={.055f,.105f,.16f,1};
    vertices.push_back({0,0,top.r,top.g,top.b,1});vertices.push_back({float(width),0,top.r,top.g,top.b,1});
    vertices.push_back({float(width),float(height),bottom.r,bottom.g,bottom.b,1});
    vertices.push_back({0,0,top.r,top.g,top.b,1});vertices.push_back({float(width),float(height),bottom.r,bottom.g,bottom.b,1});
    vertices.push_back({0,float(height),bottom.r,bottom.g,bottom.b,1});
    const Color grid={.12f,.24f,.32f,.22f};
    const float step=std::max(40.0f,width/18.0f);
    for(float x=0;x<width;x+=step)Line(x,0,x,float(height),1,grid);
    for(float y=0;y<height;y+=step)Line(0,y,float(width),y,1,grid);
    const float travel=float(std::fmod(std::max(0.0,elapsed)*18.0,width+200.0))-100;
    Line(travel,height*.78f,travel+90,height*.78f,2,{.25f,.66f,.72f,.40f});
    Line(0,height-5,float(width),height-5,5,{.91f,.69f,.25f,1});
}
void FrontendScreen::Flush(GLFWwindow* window) {
    State state;
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glUseProgram(program);glUniform2f(resolutionUniform,float(width),float(height));
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,GLsizei(vertices.size()));
    const float characterPixels=TextRendering_CharWidth(window)*width*.5f;
    for(const auto& label:labels) {
        const auto capacity=std::size_t(std::max(0.0f,std::floor(label.maxWidth/std::max(.001f,characterPixels*label.scale))));
        TextRendering_PrintString(window,Fit(label.text,capacity),2*label.x/width-1,1-2*label.y/height,label.scale);
    }
}
void FrontendScreen::Draw(GLFWwindow* window,const FrontendModel& model,double elapsed) {
    glfwGetWindowSize(window,&width,&height);if(width<=0||height<=0)return;
    vertices.clear();labels.clear();Background(elapsed);
    const bool tiny=height<420,compact=width<650;
    const float margin=std::min(40.0f,width*.05f);
    const Page page=model.CurrentPage();
    const char* heading=page==Page::Aircraft?"AERONAVES":page==Page::Controls?"CONTROLES":
                        page==Page::Pause?"VOO PAUSADO":"CAELUM";
    Text(margin,tiny?34.0f:92.0f,heading,tiny?2.0f:page==Page::Main?4.6f:2.8f,width-2*margin);
    if(!tiny)Text(margin,126,page==Page::Aircraft?"Escolha a aeronave para sua proxima missao.":
                    page==Page::Controls?"Seu voo, suas perspectivas.":
                    compact?model.SelectedAircraft().name:
                    page==Page::Pause?"A simulacao esta pausada.":"SIMULADOR DE VOO  /  RIO DE JANEIRO",1.05f,width-2*margin);
    else if(page==Page::Main||page==Page::Pause)Text(margin,54,model.SelectedAircraft().name,.9f,width-2*margin);
    if((page==Page::Main||page==Page::Pause)&&!tiny&&!compact) {
        Text(margin,183,"AERONAVE SELECIONADA",.9f,width*.39f);
        Text(margin,208,model.SelectedAircraft().name,1.3f,width*.39f);
        const Rect panel(width*.46f,158,width*.49f,std::max(150.0f,float(height)-240));
        Rectangle(panel,{.075f,.14f,.20f,.92f});
        Line(panel.x,panel.y,panel.x+panel.width,panel.y,2,{.26f,.53f,.62f,1});
        const bool light=model.SelectedAircraft().id!="a320";
        Plane(panel.x+panel.width*.52f,panel.y+panel.height*.40f+float(std::sin(elapsed*.8))*3,
              std::min(panel.width*.39f,panel.height*.34f),light,{.77f,.85f,.89f,1});
        Text(panel.x+24,panel.y+panel.height-92,"SANTOS DUMONT  ->  GALEAO",1.25f,panel.width-48);
        Line(panel.x+24,panel.y+panel.height-70,panel.x+panel.width-24,panel.y+panel.height-70,2,{.90f,.69f,.25f,1});
        Text(panel.x+24,panel.y+panel.height-37,model.SelectedAircraft().description,1.0f,panel.width-48);
    }
    if(page==Page::Controls) {
        const char* controls[]={"W / S: ajustar potencia","Botao direito: mover a camera","C / F / L: cockpit / livre / externa",
            "WASD + Q / E: mover camera livre","M: mostrar ou ocultar destino","H / B: HUD / volumes de colisao",
            "Enter: reiniciar tentativa","Esc: pausar ou voltar"};
        const int count=tiny?6:8;
        for(int i=0;i<count;++i)Text(margin,(tiny?64.0f:166.0f)+i*(tiny?20.0f:height<520?27.0f:36.0f),
                                    controls[i],tiny?.83f:1.15f,width-2*margin);
    }
    const auto buttons=model.Layout(width,height);
    for(std::size_t i=0;i<buttons.size();++i) {
        const auto& button=buttons[i];const auto& b=button.bounds;
        const bool focused=button.enabled&&(model.FocusedIndex()==int(i)||model.HoveredIndex()==int(i));
        const bool selected=button.control==Control::ChooseAircraft&&button.aircraftIndex==model.SelectedIndex();
        const Color border=focused||selected?Color{.96f,.74f,.28f,1}:Color{.19f,.32f,.41f,1};
        Rectangle(b,border);Rectangle({b.x+2,b.y+2,b.width-4,b.height-4},
            !button.enabled?Color{.065f,.08f,.10f,1}:focused?Color{.16f,.25f,.31f,1}:Color{.075f,.14f,.20f,1});
        if(focused||selected)Rectangle({b.x,b.y,4,b.height},{.96f,.74f,.28f,1});
        if(button.control==Control::ChooseAircraft) {
            const auto& choice=model.Choices()[button.aircraftIndex];
            const bool shortCard=b.height<70;
            Text(b.x+16,b.y+(shortCard?26.0f:34.0f),choice.name,shortCard?1.0f:1.45f,b.width-32);
            if(!shortCard) {
                if(b.height>150) {
                    Plane(b.x+b.width*.52f,b.y+b.height*.51f,std::min(b.width*.30f,b.height*.22f),
                          choice.id!="a320",{.77f,.85f,.89f,choice.available?1.0f:.35f});
                    Text(b.x+16,b.y+b.height-49,choice.description,1.0f,b.width-32);
                } else Text(b.x+16,b.y+60,choice.description,.95f,b.width-32);
                Text(b.x+16,b.y+b.height-18,!choice.available?"INDISPONIVEL":selected?"SELECIONADA":"CLIQUE PARA SELECIONAR",.85f,b.width-32);
            }
        } else Text(b.x+16,b.y+b.height*.5f+5,button.label,tiny?.93f:1.22f,b.width-28);
    }
    if(!model.Error().empty())Text(margin,height-(tiny?24.0f:58.0f),model.Error(),tiny?.9f:1.0f,width-2*margin);
    if(!tiny)Text(margin,height-25,"Mouse ou setas / Tab: escolher  |  Enter: confirmar  |  Esc: voltar",.9f,width-2*margin);
    Flush(window);
}
void FrontendScreen::DrawLoading(GLFWwindow* window,double fraction,const std::string& stage,
                                 const std::string& message,double elapsed) {
    glfwGetWindowSize(window,&width,&height);if(width<=0||height<=0)return;
    vertices.clear();labels.clear();Background(elapsed);
    const bool tiny=height<420;
    const float margin=tiny?std::min(40.0f,width*.05f):std::max(40.0f,width*.12f);
    const float contentWidth=width-2*margin;
    const float titleY=tiny?42.0f:height*.27f;
    Text(margin,titleY,"PREPARANDO VOO",tiny?1.65f:2.7f,contentWidth);
    if(!tiny)Text(margin,titleY+35,"Santos Dumont  ->  Galeao",1.15f,contentWidth);
    const float progressY=tiny?height*.56f:height*.58f;
    Text(margin,progressY-45,stage,tiny?1.1f:1.5f,contentWidth-65);
    Text(margin,progressY-18,message,tiny?.85f:1.05f,contentWidth);
    const double safe=std::isfinite(fraction)?std::max(0.0,std::min(1.0,fraction)):0;
    Rectangle({margin,progressY,contentWidth,12},{.16f,.25f,.31f,1});
    Rectangle({margin,progressY,contentWidth*float(safe),12},{.96f,.74f,.28f,1});
    char percent[16];std::snprintf(percent,sizeof(percent),"%3.0f%%",safe*100);
    Text(margin+contentWidth-58,progressY-45,percent,1.25f,58);
    if(!tiny)Text(margin,progressY+51,"Dica: M mostra o destino. Esc abre o menu durante o voo.",1.0f,contentWidth);
    const float buttonMargin=std::min(40.0f,width*.05f),buttonHeight=tiny?28.0f:width<650?46.0f:50.0f;
    const Rect cancel(buttonMargin,height-(tiny?40.0f:66.0f),std::min(220.0f,width-2*buttonMargin),buttonHeight);
    Rectangle(cancel,{.19f,.32f,.41f,1});Rectangle({cancel.x+2,cancel.y+2,cancel.width-4,cancel.height-4},{.075f,.14f,.20f,1});
    Text(cancel.x+16,cancel.y+cancel.height*.5f+5,"Cancelar [Esc]",tiny?.93f:1.22f,cancel.width-28);
    Flush(window);
}
} // namespace frontend
