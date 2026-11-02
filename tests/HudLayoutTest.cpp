// AI-assisted CPU regressions for responsive placement, without a window/GPU.
#include "ui/HudLayout.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
void Fits(int width,int height) {
    const HudLayout layout=CalculateHudLayout(width,height);
    Check(layout.dials.size()==5,"positive viewport preserves all five instruments");
    Check(std::isfinite(layout.scale)&&layout.scale>0&&layout.scale<=1,
          "scale is finite, positive and no larger than the preferred size");
    for(size_t i=0;i<layout.dials.size();++i) {
        const auto& d=layout.dials[i];
        Check(static_cast<int>(d.instrument)==static_cast<int>(i),"instrument order remains stable");
        Check(std::isfinite(d.x)&&std::isfinite(d.y)&&std::isfinite(d.radius)&&d.radius>0,
              "circle geometry is finite and positive");
        Check(d.x-d.radius>=-.001f&&d.x+d.radius<=width+.001f,
              "entire dial stays within the horizontal viewport");
        Check(d.y-d.radius>=-.001f&&d.y+d.radius<=height+.001f,
              "entire dial stays within the vertical viewport");
        for(size_t j=0;j<i;++j) {
            const auto& other=layout.dials[j];
            const float distance=std::hypot(d.x-other.x,d.y-other.y);
            Check(distance+.001f>=d.radius+other.radius,"instrument circles never overlap");
        }
    }
    Check(std::abs(layout.dials[2].radius/layout.dials[0].radius-1.2f)<.00001f,
          "artificial horizon remains twenty percent larger");
    if(width>=650) {
        for(const auto& d:layout.dials)
            Check(std::abs(d.y-layout.dials[0].y)<.001f,"wide windows use one aligned row");
    } else {
        Check(layout.dials[0].y==layout.dials[1].y&&
              layout.dials[2].y==layout.dials[3].y&&
              layout.dials[3].y==layout.dials[4].y&&
              layout.dials[0].y<layout.dials[2].y,"compact windows use two rows in reading order");
    }
}
}
int main() {
    const auto speed=FlightInstrument::Speed,throttle=FlightInstrument::Throttle;
    const auto altitude=FlightInstrument::Altitude,compass=FlightInstrument::Compass;
    Check(HudNeedleDegrees(speed,-10)==HudNeedleDegrees(speed,0),"negative speed saturates at zero");
    Check(HudNeedleDegrees(speed,1200)==HudNeedleDegrees(speed,1000),"overspeed preserves the final scale position");
    Check(HudNeedleDegrees(throttle,-10)==HudNeedleDegrees(throttle,0)&&
          HudNeedleDegrees(throttle,150)==HudNeedleDegrees(throttle,100),"throttle pointer stays within its scale");
    Check(std::abs(HudNeedleDegrees(speed,500)-360)<1e-9,"half-scale speed points upward");
    Check(std::abs(HudNeedleDegrees(speed,1000)-HudNeedleDegrees(speed,0)-300)<1e-9&&
          std::abs(HudNeedleDegrees(throttle,100)-HudNeedleDegrees(throttle,0)-300)<1e-9,
          "bounded instruments cover 300 degrees with distinct zero and maximum");
    Check(HudNeedleDegrees(throttle,5.01)>HudNeedleDegrees(throttle,5)&&
          HudNeedleDegrees(throttle,5.01)<HudNeedleDegrees(throttle,5.02),
          "fractional throttle readings move between printed graduations");
    Check(HudNeedleDegrees(speed,216.1)>HudNeedleDegrees(speed,216)&&
          HudNeedleDegrees(speed,216.1)<HudNeedleDegrees(speed,216.2),
          "fractional speed readings never snap to integer or tick positions");
    Check(std::abs(HudNeedleDegrees(compass,90)-90)<1e-9&&
          std::abs(HudNeedleDegrees(compass,270)-270)<1e-9,"east points right and west points left");
    Check(HudNeedleDegrees(compass,360)==HudNeedleDegrees(compass,0),"compass wraps north continuously");
    Check(HudNeedleDegrees(altitude,250)==90&&HudNeedleDegrees(altitude,500)==180&&
          HudNeedleDegrees(altitude,750)==270,"altimeter uses the complete circle");
    Check(HudNeedleDegrees(altitude,1000)==HudNeedleDegrees(altitude,0)&&
          HudNeedleDegrees(altitude,12345)==HudNeedleDegrees(altitude,345),"altimeter repeats each thousand meters");
    Check(HudNeedleDegrees(altitude,-10)==HudNeedleDegrees(altitude,990),"below-zero altitude turns backward naturally");
    const double radians=std::acos(-1.0)/180;
    const double before=HudNeedleDegrees(altitude,999.99)*radians;
    const double after=HudNeedleDegrees(altitude,1000.01)*radians;
    Check(std::hypot(std::sin(after)-std::sin(before),std::cos(after)-std::cos(before))<.0002,
          "altimeter pointer never jumps when crossing a thousand-meter boundary");
    bool rejected=false;
    try {HudNeedleDegrees(speed,std::numeric_limits<double>::infinity());}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"nonfinite input is rejected instead of producing invalid geometry");
    rejected=false;
    try {HudNeedleDegrees(FlightInstrument::Attitude,0);}
    catch(const std::invalid_argument&) {rejected=true;}
    Check(rejected,"artificial horizon cannot be mistaken for a needle instrument");
    for(const auto& dimensions:{
            std::pair<int,int>{320,240},{640,480},{649,480},{650,480},{1280,720},
            {1920,1080},{3440,1440},{2560,720},{300,900},{1000,100},{80,60},{1,1}})
        Fits(dimensions.first,dimensions.second);
    const auto hd=CalculateHudLayout(1280,720);
    Check(std::abs(hd.dials[0].radius-64)<.001f,"720p uses preferred ordinary radius");
    Check(std::abs(hd.dials[2].x-640)<.001f,"720p horizon is horizontally centered");
    Check(std::abs(hd.dials[2].y+hd.dials[2].radius-696)<.001f,"720p keeps a24pixel bottom margin");
    for(const auto& dimensions:{std::pair<int,int>{0,720},{1280,0},{-1,720},{1280,-1}}) {
        const auto empty=CalculateHudLayout(dimensions.first,dimensions.second);
        Check(empty.dials.empty()&&empty.scale==0,"nonpositive viewport produces an empty layout");
    }
    std::printf("PASS: %d responsive HUD placement and instrument checks\n",checks);
}
