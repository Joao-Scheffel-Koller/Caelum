// AI-assisted regressions: real hit targets and state routing, no window/GPU.
#include "ui/FrontendModel.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>
using namespace frontend;
namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
std::vector<AircraftChoice> Choices() {
    return {{"a320","A320-200","Jato comercial",true},{"light","Monomotor leve","Aviacao geral",true},
            {"missing","Indisponivel","Sem modelo",false}};
}
Action ClickControl(FrontendModel& menu,Control control,int w=1280,int h=720,std::size_t aircraft=0) {
    for(const auto& button:menu.Layout(w,h))if(button.control==control&&
        (control!=Control::ChooseAircraft||button.aircraftIndex==aircraft))
        return menu.Click(button.bounds.x+button.bounds.width*.5,button.bounds.y+button.bounds.height*.5,w,h);
    Check(false,"requested control exists");return Action::None;
}
void Fits(FrontendModel& menu,int width,int height) {
    const auto buttons=menu.Layout(width,height);
    Check(!buttons.empty(),"a positive viewport preserves controls");
    for(std::size_t i=0;i<buttons.size();++i) {
        const auto& a=buttons[i].bounds;
        Check(std::isfinite(a.x)&&std::isfinite(a.y)&&a.width>0&&a.height>0,"hit targets have finite positive area");
        Check(a.x>=0&&a.y>=0&&a.x+a.width<=width+.001f&&a.y+a.height<=height+.001f,"hit targets stay in the viewport");
        for(std::size_t j=0;j<i;++j) {
            const auto& b=buttons[j].bounds;
            Check(a.x+a.width<=b.x+.001f||b.x+b.width<=a.x+.001f||a.y+a.height<=b.y+.001f||b.y+b.height<=a.y+.001f,
                  "hit targets never overlap");
        }
    }
}
}
int main() {
    FrontendModel menu(Choices());
    Check(menu.Active()&&menu.CurrentPage()==Page::Main&&menu.SelectedIndex()==0,"startup opens main menu with first available aircraft");
    Check(menu.KeyPress(Key::Enter,1280,720)==Action::Start&&menu.Active(),"start returns command without prematurely closing menu");
    ClickControl(menu,Control::Aircraft);
    Check(menu.CurrentPage()==Page::Aircraft,"aircraft navigation opens selection page");
    ClickControl(menu,Control::ChooseAircraft,1280,720,1);
    Check(menu.SelectedIndex()==1&&menu.SelectedAircraft().id=="light","card click actually selects the second aircraft");
    ClickControl(menu,Control::ChooseAircraft,1280,720,2);
    Check(menu.SelectedIndex()==1,"disabled aircraft cannot replace valid selection");
    Check(ClickControl(menu,Control::Start)==Action::Start&&menu.SelectedIndex()==1,"selected aircraft reaches start action");
    menu.KeyPress(Key::Escape,1280,720);
    Check(menu.CurrentPage()==Page::Main,"selection escape returns to original page");
    ClickControl(menu,Control::Controls);Check(menu.CurrentPage()==Page::Controls,"controls page opens");
    menu.KeyPress(Key::Escape,1280,720);Check(menu.CurrentPage()==Page::Main,"controls escape returns main");
    Check(menu.KeyPress(Key::Escape,1280,720)==Action::Quit,"main escape requests quit");
    menu.OpenPause();Check(menu.KeyPress(Key::Enter,1280,720)==Action::Resume,"pause default action resumes");
    ClickControl(menu,Control::Aircraft);menu.KeyPress(Key::Escape,1280,720);
    Check(menu.CurrentPage()==Page::Pause,"selection opened from pause returns pause");
    Check(ClickControl(menu,Control::MainMenu)==Action::MainMenu,"pause can request main menu");
    menu.OpenMain();menu.BeginLoading();
    Check(menu.CurrentPage()==Page::Loading&&menu.Active(),"loading keeps input routed to frontend");
    Check(menu.KeyPress(Key::Escape,1280,720)==Action::CancelLoading,"escape while loading requests cancellation only");
    Check(ClickControl(menu,Control::CancelLoading)==Action::CancelLoading,"loading cancel button has matching hit target");
    Check(menu.SelectedIndex()==1,"loading input cannot change selected aircraft");
    menu.Close();Check(!menu.Active()&&menu.KeyPress(Key::Escape,1280,720)==Action::None,"closed frontend does not intercept gameplay");
    Check(menu.Click(10,10,1280,720)==Action::None,"closed frontend ignores mouse actions");
    menu.SetError("Falha ao carregar");Check(menu.Active()&&menu.CurrentPage()==Page::Main&&!menu.Error().empty(),"failure recovers to a usable main menu");
    menu.OpenMain();Check(menu.Error().empty()&&menu.SelectedIndex()==1,"opening main clears error and preserves choice");
    menu.Move(60,250,1280,720);Check(menu.HoveredIndex()>=0,"pointer hover tracks actual hit target");
    menu.Move(std::numeric_limits<double>::quiet_NaN(),0,1280,720);Check(menu.HoveredIndex()==-1,"invalid pointer coordinates do not focus a control");
    menu.OpenMain();menu.KeyPress(Key::Tab,1280,720);Check(menu.FocusedIndex()==1,"tab advances keyboard focus");
    menu.KeyPress(Key::Tab,1280,720,true);Check(menu.FocusedIndex()==0,"shift-tab reverses keyboard focus");
    Check(menu.Click(-100,-100,1280,720)==Action::None,"click outside controls cannot activate an action");
    for(const auto& size:{std::make_pair(1280,720),std::make_pair(800,600),std::make_pair(640,480),
                          std::make_pair(320,240),std::make_pair(320,360),std::make_pair(800,400)}) {
        // Production has two choices; each page must fit its actual viewport.
        FrontendModel responsive({Choices()[0],Choices()[1]});
        Fits(responsive,size.first,size.second);
        ClickControl(responsive,Control::Aircraft,size.first,size.second);Fits(responsive,size.first,size.second);
        responsive.OpenPause();Fits(responsive,size.first,size.second);
        ClickControl(responsive,Control::Controls,size.first,size.second);Fits(responsive,size.first,size.second);
        responsive.BeginLoading();Fits(responsive,size.first,size.second);
    }
    Check(menu.Layout(0,720).empty()&&menu.Layout(1280,0).empty(),"minimized viewport has no stale targets");
    FrontendModel none({{"missing","Indisponivel","",false}});
    Check(none.KeyPress(Key::Enter,1280,720)==Action::None,"unavailable-only catalog cannot start a nonexistent aircraft");
    none.KeyPress(Key::Down,1280,720);Check(none.FocusedIndex()==1,"keyboard skips a disabled start action");
    bool rejected=false;try {FrontendModel empty({});}catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"empty catalog is rejected before dereferencing selection");
    std::printf("PASS: %d frontend routing, selection and responsive hit-target checks\n",checks);
}
