#include "stdafx.h"

#include "Disruptor/Sequence.h"
#include "Disruptor/Util.h"


using namespace Disruptor;


class UtilTests : public ::testing::Test
{
};

TEST_F(UtilTests, ShouldReturnNextPowerOfTwo)
{
    auto powerOfTwo = Util::ceilingNextPowerOfTwo(1000);

    EXPECT_EQ(1024, powerOfTwo);
}

TEST_F(UtilTests, ShouldReturnExactPowerOfTwo)
{
    auto powerOfTwo = Util::ceilingNextPowerOfTwo(1024);

    EXPECT_EQ(1024, powerOfTwo);
}

TEST_F(UtilTests, ShouldReturnMinimumSequence)
{
    EXPECT_EQ(4L, Util::getMinimumSequence({ std::make_shared< Sequence >(11), std::make_shared< Sequence >(4), std::make_shared< Sequence >(13) }));
}

TEST_F(UtilTests, ShouldReturnLongMaxWhenNoEventProcessors)
{
    EXPECT_EQ(std::numeric_limits< std::int64_t >::max(), Util::getMinimumSequence({ }));
}

