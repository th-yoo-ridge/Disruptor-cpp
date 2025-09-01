#include "stdafx.h"
#include "LatencyRecorder.h"

#include "Disruptor.TestTools/DurationHumanizer.h"

#include <algorithm>
#include <ranges>
#include <cmath>
#include <iostream>

namespace Disruptor
{
namespace Tests
{

    struct DurationPrinter
    {
        explicit DurationPrinter(std::int64_t nanoseconds)
            : value(nanoseconds)
        {
        }

        std::int64_t value;
    };

    std::ostream& operator<<(std::ostream& stream, const DurationPrinter& printer)
    {
        auto humanDuration = Tests::DurationHumanizer::deduceHumanDuration(std::chrono::nanoseconds(printer.value));

        return stream << humanDuration.value << " " << humanDuration.shortUnitName;
    }


    // LatencyRecorder::LatencyRecorder(std::int64_t sampleSize)
    //     : m_accumulator(boost::accumulators::tag::tail< boost::accumulators::right >::cache_size = sampleSize)
    // {
    // }
    LatencyRecorder::LatencyRecorder(std::int64_t sampleSize)
    {
        m_values.reserve(static_cast<std::size_t>(sampleSize));
    }

    // void LatencyRecorder::record(std::int64_t value)
    // {
    //     m_accumulator(value);
    // }
    void LatencyRecorder::record(std::int64_t value)
    {
        m_values.push_back(static_cast<double>(value));
        m_sum += value;
        m_min = !m_min || value < *m_min ? value : *m_min;
        m_max = !m_max || value > *m_max ? value : *m_max;
    }

    // void LatencyRecorder::writeReport(std::ostream& stream) const
    // {
    //     stream
    //         << "min: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::min(m_accumulator)))
    //         << ", mean: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::mean(m_accumulator)))
    //         << ", max: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::max(m_accumulator)))
    //         << ", Q99.99: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.9999)))
    //         << ", Q99.9: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.999)))
    //         << ", Q99: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.99)))
    //         << ", Q98: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.98)))
    //         << ", Q95: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.95)))
    //         << ", Q93: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.93)))
    //         << ", Q90: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.90)))
    //         << ", Q50: " << DurationPrinter(static_cast< std::int64_t >(boost::accumulators::quantile(m_accumulator, boost::accumulators::quantile_probability = 0.50)))
    //         ;
    // }
    void LatencyRecorder::writeReport(std::ostream& stream) const
    {
        if (m_values.empty())
        {
            stream << "No latency data recorded.\n";
            return;
        }

        std::vector<double> sorted = m_values;
        std::ranges::sort(sorted);

        auto percentile = [&](double p) -> std::int64_t {
            std::size_t index = static_cast<std::size_t>(std::clamp(p, 0.0, 1.0) * (sorted.size() - 1));
            return static_cast<std::int64_t>(sorted[index]);
        };

        double mean = static_cast<double>(m_sum) / m_values.size();

        stream
            << "min: " << DurationPrinter(*m_min)
            << ", mean: " << DurationPrinter(static_cast<std::int64_t>(mean))
            << ", max: " << DurationPrinter(*m_max)
            << ", Q99.99: " << DurationPrinter(percentile(0.9999))
            << ", Q99.9: "  << DurationPrinter(percentile(0.999))
            << ", Q99: "    << DurationPrinter(percentile(0.99))
            << ", Q98: "    << DurationPrinter(percentile(0.98))
            << ", Q95: "    << DurationPrinter(percentile(0.95))
            << ", Q93: "    << DurationPrinter(percentile(0.93))
            << ", Q90: "    << DurationPrinter(percentile(0.90))
            << ", Q50: "    << DurationPrinter(percentile(0.50))
            << std::endl;
    }

} // namespace Tests
} // namespace Disruptor
