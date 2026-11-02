#include "ui/HudLayout.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
const float PreferredRadius=64.0f;
const float AttitudeRatio=1.2f;
const float GapRatio=.22f;

float RadiusFactor(int index) {return index==2?AttitudeRatio:1.0f;}

void AddRow(HudLayout& layout,int begin,int end,float width,float y,float radius) {
    const float gap=radius*GapRatio;
    float rowWidth=gap*(end-begin-1);
    for(int i=begin;i<end;++i)rowWidth+=2*radius*RadiusFactor(i);
    float left=(width-rowWidth)*.5f;
    for(int i=begin;i<end;++i) {
        const float dialRadius=radius*RadiusFactor(i);
        layout.dials.push_back({static_cast<FlightInstrument>(i),left+dialRadius,y,dialRadius});
        left+=2*dialRadius+gap;
    }
}
}

HudLayout CalculateHudLayout(int logicalWidth,int logicalHeight) {
    HudLayout layout;
    if(logicalWidth<=0||logicalHeight<=0)return layout;
    const float width=static_cast<float>(logicalWidth),height=static_cast<float>(logicalHeight);
    const float margin=std::min(24.0f,std::min(width,height)*.05f);
    const bool compact=logicalWidth<650;

    // One wide row contains four ordinary dials and one 20% larger horizon.
    // Compact windows keep reading order: first two above, last three below.
    const float widthUnits=compact?(2*(AttitudeRatio+2)+2*GapRatio):
                                    (2*(4+AttitudeRatio)+4*GapRatio);
    const float heightUnits=compact?(2+2*AttitudeRatio+GapRatio):(2*AttitudeRatio);
    const float radius=std::min(PreferredRadius,
        std::min((width-2*margin)/widthUnits,(height-2*margin)/heightUnits));
    layout.scale=radius/PreferredRadius;
    const float bottomY=height-margin-radius*AttitudeRatio;
    if(compact) {
        const float topY=bottomY-radius*AttitudeRatio-radius*GapRatio-radius;
        AddRow(layout,0,2,width,topY,radius);
        AddRow(layout,2,5,width,bottomY,radius);
    } else {
        AddRow(layout,0,5,width,bottomY,radius);
    }
    return layout;
}

double HudNeedleDegrees(FlightInstrument instrument,double reading) {
    if(!std::isfinite(reading))throw std::invalid_argument("Nonfinite HUD reading");
    switch(instrument) {
    case FlightInstrument::Speed:
        return 210+300*std::max(0.0,std::min(1000.0,reading))/1000;
    case FlightInstrument::Throttle:
        return 210+300*std::max(0.0,std::min(100.0,reading))/100;
    case FlightInstrument::Altitude:
    case FlightInstrument::Compass: {
        const double period=instrument==FlightInstrument::Altitude?1000:360;
        double wrapped=std::fmod(reading,period);
        if(wrapped<0)wrapped+=period;
        return wrapped/period*360;
    }
    default:throw std::invalid_argument("Instrument has no needle");
    }
}
