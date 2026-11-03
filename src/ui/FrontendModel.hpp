#ifndef CAELUM_FRONTEND_MODEL_HPP
#define CAELUM_FRONTEND_MODEL_HPP
#include <cstddef>
#include <string>
#include <vector>

namespace frontend {
// AI-assisted CPU-only menu state and hit targets, independent of GLFW/assets.
struct AircraftChoice {
    std::string id,name,description;
    bool available=true;
    AircraftChoice()=default;
    AircraftChoice(std::string id,std::string name,std::string description,bool available=true);
};
enum class Page { Main,Aircraft,Controls,Pause,Loading };
enum class Action { None,Start,Resume,MainMenu,Quit,CancelLoading };
enum class Key { Up,Down,Left,Right,Tab,Enter,Escape };
enum class Control { Start,Aircraft,Controls,Quit,Resume,MainMenu,Back,ChooseAircraft,CancelLoading };
struct Rect {
    float x=0,y=0,width=0,height=0;
    Rect()=default;
    Rect(float x,float y,float width,float height):x(x),y(y),width(width),height(height) {}
    bool Contains(double px,double py) const;
};
struct Button {
    Rect bounds;
    std::string label;
    Control control=Control::Back;
    std::size_t aircraftIndex=0;
    bool enabled=true;
};
class FrontendModel {
public:
    explicit FrontendModel(std::vector<AircraftChoice> choices);
    bool Active() const {return active;}
    Page CurrentPage() const {return page;}
    std::size_t SelectedIndex() const {return selected;}
    const AircraftChoice& SelectedAircraft() const {return choices[selected];}
    const std::vector<AircraftChoice>& Choices() const {return choices;}
    const std::string& Error() const {return error;}
    int FocusedIndex() const {return focus;}
    int HoveredIndex() const {return hover;}
    void OpenMain();
    void OpenPause();
    void BeginLoading();
    void Close();
    void SetError(const std::string& message);
    std::vector<Button> Layout(int width,int height) const;
    Action KeyPress(Key key,int width,int height,bool shift=false);
    Action Click(double x,double y,int width,int height);
    void Move(double x,double y,int width,int height);
private:
    std::vector<AircraftChoice> choices;
    Page page=Page::Main,returnPage=Page::Main;
    std::size_t selected=0;
    bool active=true;
    int focus=0,hover=-1;
    std::string error;
    void ChangePage(Page target);
    Action Activate(const Button& button);
};
} // namespace frontend
#endif
