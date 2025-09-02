#include "stdafx.h"

#include "Disruptor/SpinWaitWaitStrategy.h"
#include "WaitStrategyTestUtil.h"


using namespace Disruptor;
using namespace Disruptor::Tests;


class SpinWaitWaitStrategyTests : public ::testing::Test
{
};

TEST_F(SpinWaitWaitStrategyTests, ShouldWaitForValue)
{
    assertWaitForWithDelayOf(50, std::make_shared< SpinWaitWaitStrategy >());
}
