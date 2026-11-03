// AI-assisted regression checks for loading concurrency and cancellation.
#include "platform/LoadProgress.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {
int checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(EXIT_FAILURE);}
}
void Delay() {std::this_thread::sleep_for(std::chrono::milliseconds(60));}
}
int main() {
    const auto caller=std::this_thread::get_id();
    std::thread::id worker;
    int pumps=0;bool correctPumpThread=true;
    LoadCallbacks callbacks;
    callbacks.pump=[&](){++pumps;correctPumpThread=correctPumpThread&&std::this_thread::get_id()==caller;};
    const int value=RunLoadCpu(callbacks,[&](){worker=std::this_thread::get_id();Delay();return 42;});
    Check(value==42,"worker result reaches the caller");
    Check(worker!=caller,"CPU preparation leaves the graphics thread");
    Check(correctPumpThread,"event/graphics pumps remain on the caller thread");
    Check(pumps>=3,"the UI is pumped before, during and after CPU work");

    const auto inlineThread=RunLoadCpu(LoadCallbacks{},[](){return std::this_thread::get_id();});
    Check(inlineThread==caller,"headless/default loading works without an extra worker");

    std::atomic<bool> finished(false);pumps=0;
    callbacks.pump=[&](){if(++pumps>=2)throw LoadCancelled();};
    bool cancelled=false;
    try {RunLoadCpu(callbacks,[&](){Delay();finished=true;return 1;});}
    catch(const LoadCancelled&) {cancelled=true;}
    Check(cancelled,"cancellation propagates to the application");
    Check(finished,"cancellation drains the worker before the scene can be destroyed");
    Check(pumps>=2,"cancellation can be requested while preparation runs");

    finished=false;callbacks.pump=[](){};
    bool preparationError=false;
    try {RunLoadCpu(callbacks,[&]()->int {Delay();finished=true;throw std::runtime_error("parse-failed");});}
    catch(const std::runtime_error& error) {preparationError=std::string(error.what())=="parse-failed";}
    Check(preparationError&&finished,"CPU failures are rethrown after their task finishes");

    finished=false;pumps=0;callbacks.pump=[&](){if(++pumps>=2)throw LoadCancelled();};
    cancelled=false;
    try {RunLoadCpu(callbacks,[&]()->int {Delay();finished=true;throw std::runtime_error("parse-failed");});}
    catch(const LoadCancelled&) {cancelled=true;}
    Check(cancelled&&finished,"user cancellation keeps priority over a later worker failure");

    bool started=false;callbacks.pump=[](){throw LoadCancelled();};cancelled=false;
    try {RunLoadCpu(callbacks,[&](){started=true;return 1;});}
    catch(const LoadCancelled&) {cancelled=true;}
    Check(cancelled&&!started,"cancellation before starting does not launch a worker");

    finished=false;callbacks.pump=[&](){if(finished)throw LoadCancelled();};cancelled=false;
    try {RunLoadCpu(callbacks,[&](){Delay();finished=true;return 1;});}
    catch(const LoadCancelled&) {cancelled=true;}
    Check(cancelled&&finished,"a late cancellation still completes lifetime cleanup");
    std::printf("PASS: %d responsive loading and cancellation checks\n",checks);
}
