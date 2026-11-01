#include "platform/FrameLimiter.hpp"
#include <cmath>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

namespace {
std::chrono::steady_clock::duration Interval(double fps) {
    using Clock=std::chrono::steady_clock;
    if(!std::isfinite(fps)||fps<=0)
        throw std::invalid_argument("Frame rate must be positive and finite");
    const double seconds=1.0/fps;
    if(!std::isfinite(seconds)||seconds>=std::chrono::duration<double>(Clock::duration::max()).count())
        throw std::invalid_argument("Frame interval is too long");
    const auto interval=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
    if(interval<=Clock::duration::zero())
        throw std::invalid_argument("Frame interval is too short");
    return interval;
}
}

FrameLimiter::FrameLimiter(double fps):interval(Interval(fps)),next(Clock::now()) {
#ifdef _WIN32
    // Ordinary Sleep can round a 16.7ms wait to 31ms on Windows. A dedicated
    // high-resolution timer avoids that without changing system timer settings.
    timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
#endif
}

FrameLimiter::~FrameLimiter() {
#ifdef _WIN32
    if(timer)CloseHandle(static_cast<HANDLE>(timer));
#endif
}

void FrameLimiter::Wait() {
    while(Clock::now()<next) {
#ifdef _WIN32
        if(timer) {
            const auto remaining=std::chrono::duration_cast<std::chrono::nanoseconds>(next-Clock::now()).count();
            if(remaining<=0)break;
            // Wake slightly before the deadline to absorb scheduler overshoot.
            // Only the final fraction of a frame yields instead of sleeping.
            const auto margin=std::chrono::duration_cast<std::chrono::nanoseconds>(interval/4).count()<2000000
                ? std::chrono::duration_cast<std::chrono::nanoseconds>(interval/4).count():2000000;
            if(remaining<=margin) {std::this_thread::yield();continue;}
            LARGE_INTEGER due;
            due.QuadPart=-((remaining-margin+99)/100); // Negative: relative time, in 100ns units.
            if(SetWaitableTimer(static_cast<HANDLE>(timer),&due,0,nullptr,nullptr,FALSE) &&
               WaitForSingleObject(static_cast<HANDLE>(timer),INFINITE)==WAIT_OBJECT_0)
                continue;
        }
#endif
        // Portable fallback, also used if native timer creation/waiting fails.
        std::this_thread::sleep_until(next);
    }
    // Do not accumulate missed frames or burst after a stall/minimize.
    next=Clock::now()+interval;
}
