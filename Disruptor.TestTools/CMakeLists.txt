project(Disruptor.TestTools)
cmake_minimum_required(VERSION 2.6)

include_directories("..")

set(DisruptorTestTools_sources

    CountdownEvent.cpp
    ManualResetEvent.cpp
    ResetEvent.cpp
    ScopeExitFunctor.cpp
    Stopwatch.cpp
    LatencyRecorder.cpp
)

add_library(Disruptor.TestTools STATIC ${DisruptorTestTools_sources})
