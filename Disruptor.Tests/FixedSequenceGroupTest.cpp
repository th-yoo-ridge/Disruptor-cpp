#include "stdafx.h"

#include "Disruptor/FixedSequenceGroup.h"
#include "Disruptor/Sequence.h"


using namespace Disruptor;


class FixedSequenceGroupTest : public ::testing::Test
{
};

TEST_F(FixedSequenceGroupTest, ShouldReturnMinimumOf2Sequences)
{
    auto sequence1 = std::make_shared< Sequence >(34);
    auto sequence2 = std::make_shared< Sequence >(47);
    FixedSequenceGroup group({ sequence1, sequence2 });

    EXPECT_EQ(group.value(), 34L);
    sequence1->setValue(35);
    EXPECT_EQ(group.value(), 35L);
    sequence1->setValue(48);
    EXPECT_EQ(group.value(), 47L);
}

