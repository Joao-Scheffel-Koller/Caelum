#include "ui/FrontendModel.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace frontend {
AircraftChoice::AircraftChoice(std::string id,std::string name,std::string description,bool available)
    :id(std::move(id)),name(std::move(name)),description(std::move(description)),available(available) {}
bool Rect::Contains(double px,double py) const {
    return width>0&&height>0&&std::isfinite(px)&&std::isfinite(py)&&
           px>=x&&py>=y&&px<x+width&&py<y+height;
}
FrontendModel::FrontendModel(std::vector<AircraftChoice> catalog):choices(std::move(catalog)) {
    if(choices.empty())throw std::invalid_argument("Frontend needs at least one aircraft");
    for(std::size_t i=0;i<choices.size();++i)if(choices[i].available) {selected=i;break;}
}
void FrontendModel::ChangePage(Page target) {page=target;focus=0;hover=-1;}
void FrontendModel::OpenMain() {active=true;returnPage=Page::Main;error.clear();ChangePage(Page::Main);}
void FrontendModel::OpenPause() {active=true;returnPage=Page::Pause;error.clear();ChangePage(Page::Pause);}
void FrontendModel::BeginLoading() {active=true;error.clear();ChangePage(Page::Loading);}
void FrontendModel::Close() {active=false;hover=-1;}
void FrontendModel::SetError(const std::string& message) {OpenMain();error=message;}

std::vector<Button> FrontendModel::Layout(int w,int h) const {
    std::vector<Button> result;
    if(w<=0||h<=0)return result;
    const float width=float(w),height=float(h),margin=std::min(40.0f,width*.05f);
    const bool tiny=h<420,compact=w<650;
    const auto add=[&](Rect bounds,const std::string& label,Control control,bool enabled,std::size_t choice) {
        Button button;button.bounds=bounds;button.label=label;button.control=control;
        button.enabled=enabled;button.aircraftIndex=choice;result.push_back(button);
    };
    const float buttonHeight=tiny?28.0f:compact?46.0f:50.0f,gap=tiny?5.0f:10.0f;
    if(page==Page::Loading) {
        add({margin,height-(tiny?40.0f:66.0f),std::min(220.0f,width-2*margin),buttonHeight},
            "Cancelar [Esc]",Control::CancelLoading,true,0);
    } else if(page==Page::Aircraft) {
        const float firstY=tiny?65.0f:compact?150.0f:160.0f;
        const float footer=tiny?height-40.0f:height-90.0f;
        const bool rows=compact;
        const float cardGap=tiny?6.0f:16.0f;
        const float cardHeight=rows?std::max(24.0f,std::min(tiny?42.0f:105.0f,
            (footer-firstY-12-cardGap*float(choices.size()-1))/float(choices.size()))):
            std::max(32.0f,std::min(270.0f,footer-firstY-24));
        const float cardWidth=rows?width-2*margin:
            (width-2*margin-cardGap*float(choices.size()-1))/float(choices.size());
        for(std::size_t i=0;i<choices.size();++i)
            add({margin+(rows?0:float(i)*(cardWidth+cardGap)),firstY+(rows?float(i)*(cardHeight+cardGap):0),
                 cardWidth,cardHeight},choices[i].name,Control::ChooseAircraft,choices[i].available,i);
        const float backWidth=std::min(150.0f,(width-2*margin)*.3f);
        add({margin,footer,backWidth,buttonHeight},"Voltar",Control::Back,true,0);
        add({margin+backWidth+gap,footer,width-2*margin-backWidth-gap,buttonHeight},
            tiny?"Iniciar missao":"Voar com esta aeronave",Control::Start,choices[selected].available,0);
    } else if(page==Page::Controls) {
        add({margin,height-(tiny?38.0f:90.0f),std::min(230.0f,width-2*margin),buttonHeight},"Voltar [Esc]",Control::Back,true,0);
    } else {
        const int count=page==Page::Pause?5:4;
        const float listHeight=count*buttonHeight+(count-1)*gap;
        const float firstY=tiny?64.0f:std::min(compact?150.0f:230.0f,height-60-listHeight);
        const float buttonWidth=compact?width-2*margin:std::min(350.0f,width*.39f);
        int index=0;
        const auto item=[&](const std::string& label,Control control,bool enabled) {
            add({margin,firstY+index++*(buttonHeight+gap),buttonWidth,buttonHeight},label,control,enabled,0);
        };
        if(page==Page::Pause) {
            item("Continuar voo",Control::Resume,true);
            item("Trocar aeronave",Control::Aircraft,true);
            item("Controles",Control::Controls,true);
            item("Menu inicial",Control::MainMenu,true);
            item("Sair",Control::Quit,true);
        } else {
            item("Iniciar missao",Control::Start,choices[selected].available);
            item("Selecionar aeronave",Control::Aircraft,true);
            item("Controles",Control::Controls,true);
            item("Sair",Control::Quit,true);
        }
    }
    return result;
}

Action FrontendModel::Activate(const Button& button) {
    if(!button.enabled)return Action::None;
    switch(button.control) {
    case Control::Start:return Action::Start;
    case Control::Resume:return Action::Resume;
    case Control::MainMenu:return Action::MainMenu;
    case Control::Quit:return Action::Quit;
    case Control::CancelLoading:return Action::CancelLoading;
    case Control::Aircraft:returnPage=page;ChangePage(Page::Aircraft);break;
    case Control::Controls:returnPage=page;ChangePage(Page::Controls);break;
    case Control::Back:ChangePage(returnPage);break;
    case Control::ChooseAircraft:selected=button.aircraftIndex;error.clear();break;
    }
    return Action::None;
}
Action FrontendModel::KeyPress(Key key,int width,int height,bool shift) {
    if(!active)return Action::None;
    if(key==Key::Escape) {
        if(page==Page::Loading)return Action::CancelLoading;
        if(page==Page::Pause)return Action::Resume;
        if(page==Page::Main)return Action::Quit;
        ChangePage(returnPage);return Action::None;
    }
    const auto buttons=Layout(width,height);
    if(buttons.empty())return Action::None;
    const int count=int(buttons.size());
    if(focus<0||focus>=count)focus=0;
    if(key==Key::Enter) {
        if(buttons[std::size_t(focus)].enabled)return Activate(buttons[std::size_t(focus)]);
        return Action::None;
    }
    const int direction=key==Key::Up||key==Key::Left||(key==Key::Tab&&shift)?-1:1;
    for(int i=0;i<count;++i) {
        focus=(focus+direction+count)%count;
        if(buttons[std::size_t(focus)].enabled)break;
    }
    hover=-1;
    return Action::None;
}
Action FrontendModel::Click(double x,double y,int width,int height) {
    if(!active)return Action::None;
    const auto buttons=Layout(width,height);
    for(std::size_t i=0;i<buttons.size();++i)if(buttons[i].enabled&&buttons[i].bounds.Contains(x,y)) {
        focus=int(i);hover=int(i);return Activate(buttons[i]);
    }
    return Action::None;
}
void FrontendModel::Move(double x,double y,int width,int height) {
    hover=-1;if(!active)return;
    const auto buttons=Layout(width,height);
    for(std::size_t i=0;i<buttons.size();++i)if(buttons[i].enabled&&buttons[i].bounds.Contains(x,y)) {
        hover=int(i);focus=int(i);break;
    }
}
} // namespace frontend
