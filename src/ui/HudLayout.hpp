#ifndef CAELUM_HUD_LAYOUT_HPP
#define CAELUM_HUD_LAYOUT_HPP
#include <vector>

enum class FlightInstrument { Speed, Altitude, Attitude, Compass, Throttle };

// AI-assisted CPU layout. Positions use logical window pixels, origin top-left.
struct HudDial {
    FlightInstrument instrument;
    float x,y,radius;
};
struct HudLayout {
    std::vector<HudDial> dials;
    float scale=0;
};

HudLayout CalculateHudLayout(int logicalWidth,int logicalHeight);
// Clockwise angle from twelve o'clock. Altitude completes one turn per 1000 m;
// speed/throttle use a continuous 300-degree sweep and saturate at their scale
// limits. Attitude has no needle. Tick marks do not quantize these angles.
double HudNeedleDegrees(FlightInstrument instrument,double reading);
#endif
