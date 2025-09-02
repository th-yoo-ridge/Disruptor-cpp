#include "stdafx.h"

#include "Disruptor/Sequence.h"
#include "Disruptor/TimeoutBlockingWaitStrategy.h"
#include "Disruptor/TimeoutException.h"

#include "SequenceBarrierMock.h"


using namespace Disruptor;


class TimeoutBlockingWaitStrategyTest : public ::testing::Test
{
};

TEST_F(TimeoutBlockingWaitStrategyTest, ShouldTimeoutWaitFor)
{
    auto sequenceBarrierMock = std::make_shared< testing::NiceMock< Tests::SequenceBarrierMock > >();

    auto theTimeout = std::chrono::milliseconds(500);
    auto waitStrategy = std::make_shared< TimeoutBlockingWaitStrategy >(theTimeout);
    auto cursor = std::make_shared< Sequence >(5);
    const auto& dependent = cursor;

    EXPECT_CALL(*sequenceBarrierMock, checkAlert()).Times(testing::AtLeast(1));

    auto t0 = ClockConfig::Clock::now();

    try
    {
        waitStrategy->waitFor(6, *cursor, *dependent, *sequenceBarrierMock);
        throw std::runtime_error("TimeoutException should have been thrown");
    }
    catch (TimeoutException&)
    {
    }

    auto t1 = ClockConfig::Clock::now();

    auto timeWaiting = t1 - t0;

    EXPECT_GE(timeWaiting, theTimeout);
}

