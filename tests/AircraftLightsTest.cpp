// CPU checks for surveyed emitter anchors and elapsed-time flash patterns.
#include "aircraft/AircraftLights.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <stdexcept>

using namespace aircraftlights;
namespace {
int checks=0;
void Check(bool condition,const char* message) {
    ++checks;
    if(!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-6) {return std::abs(a-b)<=tolerance;}
bool White(const glm::vec3& color) {return color.x>.9f&&color.y>.9f&&color.z>.9f;}
bool Red(const glm::vec3& color) {return color.x>.9f&&color.y<.1f&&color.z<.1f;}
bool Green(const glm::vec3& color) {return color.y>.9f&&color.x<.1f&&color.z<.1f;}

void Timing() {
    for(double time:{0.0,.03,.1,.19,.5,1.2,48.0})
        Check(Intensity(Pattern::Steady,time)==1,"navigation lights remain continuously on");
    Check(Intensity(Pattern::Beacon,.08)==1,"beacon reaches full intensity during its pulse");
    Check(Intensity(Pattern::Beacon,.20)==0&&Intensity(Pattern::Beacon,1.19)==0,
          "beacon remains dark between short pulses");
    Check(Intensity(Pattern::Beacon,0)==0&&Intensity(Pattern::Beacon,.16)==0,
          "beacon pulse ends at the specified boundaries");
    Check(Near(Intensity(Pattern::Beacon,.01),.5)&&Near(Intensity(Pattern::Beacon,.15),.5),
          "beacon uses a gradual rise and fall instead of a hard square wave");
    Check(Intensity(Pattern::Beacon,.005)<Intensity(Pattern::Beacon,.01)&&
          Intensity(Pattern::Beacon,.01)<Intensity(Pattern::Beacon,.015),
          "beacon transition increases monotonically");
    Check(Intensity(Pattern::Strobe,.03)==1&&Intensity(Pattern::Strobe,.19)==1,
          "each strobe cycle contains two bright flashes");
    for(double time:{0.0,.065,.10,.16,.225,.4,1.19})
        Check(Intensity(Pattern::Strobe,time)==0,"strobes are dark outside their two flash windows");
    Check(Near(Intensity(Pattern::Strobe,.004),.5)&&Near(Intensity(Pattern::Strobe,.061),.5),
          "strobe edges have short smooth transitions");
    for(Pattern pattern:{Pattern::Beacon,Pattern::Strobe}) {
        for(double time:{.004,.01,.03,.08,.15,.164,.19,.221,.5,1.18}) {
            const double value=Intensity(pattern,time);
            Check(Near(value,Intensity(pattern,time+1.2))&&Near(value,Intensity(pattern,time+120)),
                  "flash timing repeats every 1.2 seconds over many cycles");
            Check(Near(value,Intensity(pattern,time-1.2)),"negative time wraps to the same cycle");
            Check(Near(value,Intensity(pattern,0,time)),"phase offsets are expressed in elapsed seconds");
        }
        for(int i=0;i<2400;++i) {
            const float value=Intensity(pattern,i*.001,.37);
            Check(std::isfinite(value)&&value>=0&&value<=1,"all sampled flash intensities stay finite and normalized");
        }
        Check(Intensity(pattern,.03,0)!=Intensity(pattern,.03,.6),
              "different aircraft phase offsets prevent all NPC flashes from synchronizing");
    }
    const double infinity=std::numeric_limits<double>::infinity();
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(Pattern pattern:{Pattern::Beacon,Pattern::Strobe}) {
        for(double invalid:{infinity,-infinity,nan})
            Check(Intensity(pattern,invalid)==0&&Intensity(pattern,0,invalid)==0,
                  "nonfinite flash timing is safely dark");
        Check(std::isfinite(Intensity(pattern,std::numeric_limits<double>::max(),
                                     std::numeric_limits<double>::max())),
              "large finite phase values cannot overflow when combined");
    }
}

void FrameRatesAndClock() {
    // Compare the same real elapsed times, including flash transitions, instead
    // of counting frames or requiring each rate to sample every narrow flash.
    for(double seconds:{.01,.03,.08,.15,.19,.5,1.21,12.19,60.0}) {
        Clock reference;reference.Update(seconds);
        for(int fps:{30,60,144}) {
            Clock clock;
            const int frames=static_cast<int>(std::floor(seconds*fps));
            for(int frame=0;frame<frames;++frame)clock.Update(1.0/fps);
            clock.Update(seconds-double(frames)/fps);
            Check(Near(clock.Time(),reference.Time(),1e-10),
                  "30, 60 and 144 FPS accumulate the same elapsed simulation time");
            for(Pattern pattern:{Pattern::Beacon,Pattern::Strobe})
                Check(Near(Intensity(pattern,clock.Time()),Intensity(pattern,reference.Time()),1e-5),
                      "flash brightness at a fixed elapsed time is independent of frame rate");
        }
    }
    Clock clock;
    Check(clock.Time()==0,"a new light clock starts at zero");
    clock.Update(.08);
    const double paused=clock.Time();
    for(int i=0;i<100;++i)clock.Update(0);
    Check(clock.Time()==paused&&Intensity(Pattern::Beacon,clock.Time())==1,
          "zero simulation steps preserve a paused flash");
    for(double invalid:{-1.0,std::numeric_limits<double>::quiet_NaN(),
                        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
        clock.Update(invalid);
        Check(clock.Time()==paused,"negative and nonfinite deltas cannot corrupt the light clock");
    }
    clock.Reset();
    Check(clock.Time()==0,"reset starts the next flight at the beginning of the light cycle");
    clock.Update(std::numeric_limits<double>::max());
    Check(std::isfinite(clock.Time()),"a finite long elapsed time remains valid");
    const double beforeOverflow=clock.Time();
    clock.Update(std::numeric_limits<double>::max());
    Check(clock.Time()==beforeOverflow&&std::isfinite(clock.Time()),
          "overflowing clock additions are ignored rather than producing infinity");
    clock.Reset();clock.Update(5);
    Check(clock.Time()==5,"large valid steps are accumulated without a frame-sized time clamp");
}

void Geometry(const char* id,bool lightAircraft) {
    const std::string aircraftId(id);
    const auto& profile=Profile(aircraftId);
    Check(profile.size()==(lightAircraft?6u:7u),"each catalog airframe has its intended emitter count");
    const glm::vec3 low=lightAircraft?glm::vec3(-4.5f,0,-5.5f):glm::vec3(-27.425108f,-3.342473f,-17.178679f);
    const glm::vec3 high=lightAircraft?glm::vec3(4.5f,3,5.5f):glm::vec3(10.548508f,8.196299f,17.178555f);
    int navigation=0,beacons=0,strobes=0,strobeLeft=0,strobeRight=0,left=0,right=0,aft=0,top=0,bottom=0;
    for(const auto& lamp:profile) {
        const auto& p=lamp.position;
        Check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
              std::isfinite(lamp.phaseOffset),"all local light positions and phase offsets are finite");
        Check(std::isfinite(lamp.radius)&&lamp.radius>0&&lamp.radius<.1f&&
              lamp.haloRadius>=2*lamp.radius&&lamp.haloRadius<=3*lamp.radius,
              "physical emitters have small positive radii and compact two-to-three-radius halos");
        for(int axis=0;axis<3;++axis)
            Check(p[axis]>low[axis]-.15f&&p[axis]<high[axis]+.15f,
                  "surveyed emitters remain close to the actual mesh bounds");
        if(lamp.pattern==Pattern::Steady) {
            ++navigation;
            if(Red(lamp.color)) {++left;Check(p.z<0,"red navigation light is on the left wing");}
            else if(Green(lamp.color)) {++right;Check(p.z>0,"green navigation light is on the right wing");}
            else {
                Check(White(lamp.color),"aft navigation light is white");++aft;
                Check(p.x<low.x&&std::abs(p.z)<.05f,"white aft light clears the actual tail extremity");
                Check(lightAircraft?(p.y>2.75f&&p.y<2.9f):(p.y>.95f&&p.y<1.3f),
                      "aft light follows the surveyed tail height rather than a bounding-box corner");
            }
        }
        if(lamp.pattern==Pattern::Beacon) {
            ++beacons;Check(Red(lamp.color),"anti-collision beacons are red");
            Check(std::abs(p.z)<.05f,"beacons stay on the airframe centerline");
            if(lightAircraft) {
                ++top;Check(p.x>-3.9f&&p.x<-3.7f&&p.y>3&&p.y<3.15f,
                           "light-aircraft beacon clears the surveyed vertical-fin top");
            } else if(p.y>0) {
                ++top;Check(p.x>-7.5f&&p.x<-7.1f&&p.y>2.21f&&p.y<2.4f,
                           "jet top beacon sits above the surveyed fuselage dome");
            } else {
                ++bottom;Check(p.x>-8.2f&&p.x<-7.9f&&p.y<-2.315f&&p.y>-2.5f,
                              "jet bottom beacon clears the surveyed belly attachment");
            }
        }
        if(lamp.pattern==Pattern::Strobe) {
            ++strobes;Check(White(lamp.color),"wing strobes are white");
            if(p.z<0)++strobeLeft;
            if(p.z>0)++strobeRight;
        }
        if(lamp.pattern==Pattern::Strobe||(lamp.pattern==Pattern::Steady&&!White(lamp.color))) {
            Check(std::abs(p.z)>high.z&&std::abs(p.z)<high.z+.12f,
                  "wing emitters clear the mesh laterally without floating far away");
            Check(lightAircraft?(p.x>.7f&&p.x<2.8f&&p.y>2.2f&&p.y<3):
                                (p.x>-12&&p.x<-10.7f&&p.y>.65f&&p.y<.85f),
                  "wing lamps use surveyed tip chord and height instead of arbitrary AABB extremes");
        }
    }
    Check(navigation==3&&left==1&&right==1&&aft==1,"profile includes red left, green right and white aft steady lights");
    Check(strobes==2&&strobeLeft==1&&strobeRight==1,"profile has one white strobe per wing");
    Check(beacons==(lightAircraft?1:2)&&top==1&&bottom==(lightAircraft?0:1),
          "jet has top/bottom beacons and the monomotor has one fin beacon");
}
} // namespace

int main() {
    Timing();FrameRatesAndClock();Geometry("a320",false);Geometry("light",true);
    bool rejected=false;
    try {Profile("unknown-airframe");}catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"unknown meshes cannot inherit unsuitable emitter anchors silently");
    Check(&Profile("a320")==&Profile("a320"),"profile lookup retains stable immutable storage");
    std::printf("PASS: %d aircraft light geometry, timing and clock checks\n",checks);
}
