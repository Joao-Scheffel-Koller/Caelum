#include "ui/MissionOverlay.hpp"
#include "ui/HudLayout.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// FONTE: existing professor-base font renderer, src/ui/textrendering.cpp.
void TextRendering_PrintString(GLFWwindow*,const std::string&,float,float,float);
float TextRendering_CharWidth(GLFWwindow*);
float TextRendering_LineHeight(GLFWwindow*);

namespace {
bool Terminal(mission::Stage stage) {
    return stage==mission::Stage::Completed||stage==mission::Stage::Failed;
}
const char* ShortInstruction(mission::Stage stage) {
    switch(stage) {
    case mission::Stage::Departure:return "Decole do Santos Dumont";
    case mission::Stage::EnRoute:return "Siga para o Galeao";
    case mission::Stage::Approach:return "Aproxime-se da pista";
    case mission::Stage::Landing:return "Desacelere e mantenha na pista";
    case mission::Stage::Completed:return "Missao concluida!";
    case mission::Stage::Failed:return "Missao falhou";
    }
    return "Missao";
}
std::string Fit(const std::string& text,std::size_t capacity) {
    if(text.size()<=capacity)return text;
    if(capacity<=3)return text.substr(0,capacity);
    std::size_t end=capacity-3;
    const auto word=text.rfind(' ',end);
    if(word!=std::string::npos&&word>end/2)end=word;
    return text.substr(0,end)+"...";
}
std::string Distance(double meters) {
    if(!std::isfinite(meters)||meters<0)return "--";
    char buffer[48];
    if(meters>=1000)std::snprintf(buffer,sizeof(buffer),"%.1f km",meters/1000);
    else std::snprintf(buffer,sizeof(buffer),"%.0f m",meters);
    return buffer;
}
std::string Navigation(const mission::MissionSnapshot& snapshot) {
    char heading[24]="--";
    if(std::isfinite(snapshot.bearingDegrees)) {
        double degrees=std::fmod(snapshot.bearingDegrees,360.0);
        if(degrees<0)degrees+=360;
        std::snprintf(heading,sizeof(heading),"%03.0f",std::fmod(std::round(degrees),360.0));
    }
    return Distance(snapshot.distanceMeters)+" | Rumo "+heading;
}
std::string Confirmation(const mission::MissionSnapshot& snapshot) {
    const double seconds=std::isfinite(snapshot.landingSeconds)?
        std::max(0.0,std::min(2.0,snapshot.landingSeconds)):0;
    char buffer[64];
    std::snprintf(buffer,sizeof(buffer),"Confirmacao: %.1f / 2.0 s",seconds);
    return buffer;
}
std::string Tiny(const mission::MissionSnapshot& snapshot) {
    switch(snapshot.stage) {
    case mission::Stage::Departure:return "Decole | Galeao";
    case mission::Stage::EnRoute:return "Rota: "+Distance(snapshot.distanceMeters);
    case mission::Stage::Approach:return "Aproxime | Galeao";
    case mission::Stage::Landing:
        if(!std::isfinite(snapshot.landingSeconds)||snapshot.landingSeconds<=0)
            return snapshot.instruction.empty()?"Pouse | Galeao":snapshot.instruction;
        else {
            char buffer[40];
            std::snprintf(buffer,sizeof(buffer),"Pouso %.1f/2s",std::min(2.0,snapshot.landingSeconds));
            return buffer;
        }
    case mission::Stage::Completed:return "Concluida | Enter";
    case mission::Stage::Failed:return "Falhou | Enter";
    }
    return "Missao";
}
}

void MissionOverlay::Draw(GLFWwindow* window,const mission::MissionSnapshot& snapshot) const {
    int width,height;glfwGetWindowSize(window,&width,&height);
    if(width<=0||height<=0)return;
    float instrumentsTop=float(height);
    for(const auto& dial:CalculateHudLayout(width,height).dials)
        instrumentsTop=std::min(instrumentsTop,dial.y-dial.radius);
    const float charPixels=TextRendering_CharWidth(window)*width*.5f;
    if(!std::isfinite(charPixels)||charPixels<=0)return;
    const float primaryScale=1.1f,secondaryScale=1.05f;
    const float margin=12;
    const auto print=[&](const std::string& text,float x,float y,float scale,float availableWidth) {
        const auto capacity=std::size_t(std::max(0.0f,std::floor(availableWidth/(charPixels*scale))));
        TextRendering_PrintString(window,Fit(text,capacity),2*x/width-1,1-2*y/height,scale);
    };

    // The caller suppresses corner camera help in this tiny layout, leaving
    // the only available row above the dials for mission status and the FPS.
    if(width<650&&height<360) {
        const float baseline=std::min(24.0f,instrumentsTop-3);
        if(baseline>=14)print(Tiny(snapshot),margin,baseline,primaryScale,width-2*margin-96);
        return;
    }

    const bool rightColumn=width>=1100;
    const float x=rightColumn?width-330.0f:margin;
    const float availableWidth=rightColumn?318.0f:width-2*margin;
    const float firstBaseline=rightColumn?52.0f:width>=650?
        std::max(26.0f,.04f*height)+112.0f:82.0f;
    const float lineStep=std::max(22.0f,TextRendering_LineHeight(window)*height*.5f*primaryScale);
    const int room=int(std::floor((instrumentsTop-6-firstBaseline)/lineStep))+1;
    if(room<=0)return;

    std::vector<std::string> lines;
    if(room==1)lines.push_back(Tiny(snapshot));
    else if(room==2) {
        lines.push_back(snapshot.instruction.empty()?ShortInstruction(snapshot.stage):snapshot.instruction);
        lines.push_back(Terminal(snapshot.stage)?"Enter: reiniciar missao":
            snapshot.stage==mission::Stage::Landing?Confirmation(snapshot):Navigation(snapshot));
    } else {
        lines.push_back("Missao: Santos Dumont -> Galeao");
        const std::size_t capacity=std::size_t(availableWidth/(charPixels*secondaryScale));
        const std::string instruction=snapshot.instruction.empty()?ShortInstruction(snapshot.stage):snapshot.instruction;
        // Preserve the actual reason (align, slow down, hold), even when the
        // detailed instruction needs shortening in the right-hand column.
        lines.push_back(Fit(instruction,capacity));
        if(Terminal(snapshot.stage)) {
            lines.push_back("Enter: reiniciar missao");
        } else {
            if(room>=5&&!snapshot.targetName.empty())lines.push_back("Alvo: "+snapshot.targetName);
            else if(room>=4&&snapshot.stage!=mission::Stage::Landing&&!snapshot.targetName.empty())
                lines.push_back("Alvo: "+snapshot.targetName);
            lines.push_back(snapshot.stage==mission::Stage::Landing?Confirmation(snapshot):Navigation(snapshot));
            if(snapshot.stage==mission::Stage::Landing&&room>=int(lines.size())+1)
                lines.push_back(Navigation(snapshot));
        }
    }
    for(std::size_t i=0;i<lines.size()&&int(i)<room;++i)
        print(lines[i],x,firstBaseline+i*lineStep,i==0?primaryScale:secondaryScale,availableWidth);
}
