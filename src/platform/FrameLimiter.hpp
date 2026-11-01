#ifndef CAELUM_FRAME_LIMITER_HPP
#define CAELUM_FRAME_LIMITER_HPP
#include <chrono>

// AI-assisted software cap, independent of the monitor refresh rate.
class FrameLimiter {
public:
    explicit FrameLimiter(double fps=60.0);
    ~FrameLimiter();
    FrameLimiter(const FrameLimiter&)=delete;
    FrameLimiter& operator=(const FrameLimiter&)=delete;
    void Wait();
private:
    using Clock=std::chrono::steady_clock;
    Clock::duration interval;
    Clock::time_point next;
    void* timer=nullptr; // Native handle stays private to the implementation.
};
#endif
