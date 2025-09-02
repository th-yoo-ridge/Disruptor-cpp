#include "stdafx.h"

#include "Disruptor/SleepingWaitStrategy.h"
#include "WaitStrategyTestUtil.h"


using namespace Disruptor;
using namespace Disruptor::Tests;


class SleepingWaitStrategyTests : public ::testing::Test
{
};

TEST_F(SleepingWaitStrategyTests, ShouldWaitForValue)
{
    assertWaitForWithDelayOf(50, std::make_shared< SleepingWaitStrategy >());
}
