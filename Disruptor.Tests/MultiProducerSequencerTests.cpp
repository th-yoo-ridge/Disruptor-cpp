#include "stdafx.h"

#include "Disruptor/BlockingWaitStrategy.h"
#include "Disruptor/MultiProducerSequencer.h"


using namespace Disruptor;


class MultiProducerSequencerTests : public ::testing::Test
{
};

TEST_F(MultiProducerSequencerTests, ShouldOnlyAllowMessagesToBeAvailableIfSpecificallyPublished)
{
    auto waitingStrategy = std::make_shared< BlockingWaitStrategy >();
    auto publisher = std::make_shared< MultiProducerSequencer< int > >(1024, waitingStrategy);

    publisher->publish(3);
    publisher->publish(5);

    EXPECT_FALSE(publisher->isAvailable(0));
    EXPECT_FALSE(publisher->isAvailable(1));
    EXPECT_FALSE(publisher->isAvailable(2));
    EXPECT_TRUE(publisher->isAvailable(3));
    EXPECT_FALSE(publisher->isAvailable(4));
    EXPECT_TRUE(publisher->isAvailable(5));
    EXPECT_FALSE(publisher->isAvailable(6));
}

