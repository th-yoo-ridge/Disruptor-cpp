#include "stdafx.h"

#include "Disruptor/YieldingWaitStrategy.h"
#include "WaitStrategyTestUtil.h"


using namespace Disruptor;
using namespace Disruptor::Tests;


class YieldingWaitStrategyTests : public ::testing::Test
{
};

TEST_F(YieldingWaitStrategyTests, ShouldWaitForValue)
{
    assertWaitForWithDelayOf(50, std::make_shared< YieldingWaitStrategy >());
}
