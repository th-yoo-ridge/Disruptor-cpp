#pragma once

#include <cstdint>
#include <exception>
#include <iosfwd>

#include <optional>

#include "Disruptor/ClockConfig.h"
#include "Disruptor.TestTools/LatencyRecorder.h"


namespace Disruptor
{
namespace PerfTests
{

    class LatencyTestSessionResult
    {
    public:
        LatencyTestSessionResult(const std::shared_ptr< Tests::LatencyRecorder >& latencyRecorder, const ClockConfig::Duration& duration);
        
        explicit LatencyTestSessionResult(const std::exception& exception);

        std::string toString();

        ClockConfig::Duration duration() const;

    private:
        std::shared_ptr< Tests::LatencyRecorder > m_latencyRecorder;
        ClockConfig::Duration m_duration;
        std::optional< std::exception > m_exception;
    };

} // namespace PerfTests
} // namespace Disruptor
