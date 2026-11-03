#ifndef CAELUM_LOAD_PROGRESS_HPP
#define CAELUM_LOAD_PROGRESS_HPP
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <future>
#include <chrono>
#include <exception>
#include <utility>

// AI-assisted loading protocol shared by the renderer and the application.
// Counts represent completed work units, not estimated wall-clock duration.
struct LoadProgress {
    std::string stage;
    std::size_t completed=0,total=0;
    LoadProgress()=default;
    LoadProgress(const std::string& label,std::size_t done,std::size_t count)
        :stage(label),completed(done),total(count) {}
};
struct LoadCallbacks {
    std::function<void(const LoadProgress&)> progress;
    // Runs on the OpenGL/main thread, including while CPU preparation waits.
    // Throw LoadCancelled to stop after draining the current worker safely.
    std::function<void()> pump;
};
class LoadCancelled:public std::runtime_error {
public:
    LoadCancelled():std::runtime_error("Loading cancelled") {}
};

// AI-assisted CPU preparation runner. Graphics/event callbacks always execute
// on the caller's context thread; Work must contain no OpenGL calls. There is
// at most one worker per invocation, and interruption never abandons it.
template<class Work>
auto RunLoadCpu(const LoadCallbacks& callbacks,Work work)->decltype(work()) {
    if(!callbacks.pump)return work();
    callbacks.pump();
    auto task=std::async(std::launch::async,std::move(work));
    std::exception_ptr interruption;
    while(task.wait_for(std::chrono::milliseconds(16))!=std::future_status::ready) {
        try {callbacks.pump();}
        catch(...) {if(!interruption)interruption=std::current_exception();}
    }
    // A worker can still own temporary data or index a not-yet-visible world.
    // Drain it before the caller can destroy/replace that scene after cancel.
    if(interruption) {
        try {task.get();}catch(...) {}
        std::rethrow_exception(interruption);
    }
    auto result=task.get();
    callbacks.pump();
    return result;
}
#endif
