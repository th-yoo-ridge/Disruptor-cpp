#include "stdafx.h"

#include "Disruptor.TestTools/ManualResetEvent.h"

#include "SequencerFixture.h"
#include "WaitStrategyMock.h"


using namespace Disruptor;
using namespace Disruptor::Tests;


using SequencerTypes = ::testing::Types<
    MultiProducerSequencer< int >,
    SingleProducerSequencer< int >
>;

template<typename T>
class SequencerTests : public SequencerTestFixtureTyped<T>
{
};

TYPED_TEST_SUITE(SequencerTests, SequencerTypes);

// Test suite converted to Google Test TYPED_TEST_SUITE above

TYPED_TEST(SequencerTests, ShouldStartWithInitialValue)
{
    EXPECT_EQ(this->m_sequencer->next(), 0);
}

TYPED_TEST(SequencerTests, ShouldBatchClaim)
{
    EXPECT_EQ(this->m_sequencer->next(4), 3);
}

TYPED_TEST(SequencerTests, ShouldIndicateHasAvailableCapacity)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(1), true);
    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(this->m_bufferSize), true);
    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(this->m_bufferSize + 1), false);

    this->m_sequencer->publish(this->m_sequencer->next());

    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(this->m_bufferSize - 1), true);
    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(this->m_bufferSize), false);
}

TYPED_TEST(SequencerTests, ShouldIndicateNoAvailableCapacity)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    auto sequence = this->m_sequencer->next(this->m_bufferSize);
    this->m_sequencer->publish(sequence - (this->m_bufferSize - 1), sequence);

    EXPECT_EQ(this->m_sequencer->hasAvailableCapacity(1), false);
}

TYPED_TEST(SequencerTests, ShouldHoldUpPublisherWhenBufferIsFull)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    auto sequence = this->m_sequencer->next(this->m_bufferSize);
    this->m_sequencer->publish(sequence - (this->m_bufferSize - 1), sequence);

    auto waitingSignal = std::make_shared< ManualResetEvent >(false);
    auto doneSignal = std::make_shared< ManualResetEvent >(false);

    auto expectedFullSequence = Sequence::InitialCursorValue + this->m_sequencer->bufferSize();
    EXPECT_EQ(this->m_sequencer->cursor(), expectedFullSequence);

    std::thread([=]
    {
        waitingSignal->set();

        auto next = this->m_sequencer->next();
        this->m_sequencer->publish(next);

        doneSignal->set();
    }).detach();

    waitingSignal->wait(std::chrono::milliseconds(500));
    EXPECT_EQ(this->m_sequencer->cursor(), expectedFullSequence);

    this->m_gatingSequence->setValue(Sequence::InitialCursorValue + 1L);

    doneSignal->wait(std::chrono::milliseconds(500));
    EXPECT_EQ(this->m_sequencer->cursor(), expectedFullSequence + 1L);
}

TYPED_TEST(SequencerTests, ShouldThrowInsufficientCapacityExceptionWhenSequencerIsFull)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    for (auto i = 0; i < this->m_bufferSize; ++i)
    {
        this->m_sequencer->next();
    }

    EXPECT_THROW(this->m_sequencer->tryNext(), InsufficientCapacityException);
}

TYPED_TEST(SequencerTests, ShouldCalculateRemainingCapacity)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    EXPECT_EQ(this->m_sequencer->getRemainingCapacity(), this->m_bufferSize);
    
    for (auto i = 1; i < this->m_bufferSize; ++i)
    {
        this->m_sequencer->next();
        EXPECT_EQ(this->m_sequencer->getRemainingCapacity(), this->m_bufferSize - i);
    }
}

TYPED_TEST(SequencerTests, ShoundNotBeAvailableUntilPublished)
{
    auto next = this->m_sequencer->next(6);

    for (auto i = 0; i <= 5; i++)
    {
        EXPECT_EQ(this->m_sequencer->isAvailable(i), false);
    }

    this->m_sequencer->publish(next - (6 - 1), next);
    
    for (auto i = 0; i <= 5; i++)
    {
        EXPECT_EQ(this->m_sequencer->isAvailable(i), true);
    }

    EXPECT_EQ(this->m_sequencer->isAvailable(6), false);
}

TYPED_TEST(SequencerTests, ShouldNotifyWaitStrategyOnPublish)
{
    auto waitStrategyMock = std::make_shared< testing::NiceMock< WaitStrategyMock > >();
    auto sequencer = std::make_shared< TypeParam >(this->m_bufferSize, waitStrategyMock);

    EXPECT_CALL(*waitStrategyMock, signalAllWhenBlocking()).Times(1);

    sequencer->publish(sequencer->next());
}

TYPED_TEST(SequencerTests, ShouldNotifyWaitStrategyOnPublishBatch)
{
    auto waitStrategyMock = std::make_shared< testing::NiceMock< WaitStrategyMock > >();
    auto sequencer = std::make_shared< TypeParam >(this->m_bufferSize, waitStrategyMock);

    EXPECT_CALL(*waitStrategyMock, signalAllWhenBlocking()).Times(1);

    auto next = sequencer->next(4);
    sequencer->publish(next - (4 - 1), next);
}

TYPED_TEST(SequencerTests, ShouldWaitOnPublication)
{
    auto barrier = this->m_sequencer->newBarrier({});

    auto next = this->m_sequencer->next(10);
    auto lo = next - (10 - 1);
    auto mid = next - 5;

    for (auto l = lo; l < mid; ++l)
    {
       this->m_sequencer->publish(l);
    }

    EXPECT_EQ(barrier->waitFor(-1), mid - 1);

    for (auto l = mid; l <= next; ++l)
    {
        this->m_sequencer->publish(l);
    }

    EXPECT_EQ(barrier->waitFor(-1), next);
}

TYPED_TEST(SequencerTests, ShouldTryNext)
{
    this->m_sequencer->addGatingSequences({ this->m_gatingSequence });

    for (auto i = 0; i < this->m_bufferSize; i++)
    {
        EXPECT_NO_THROW(this->m_sequencer->publish(this->m_sequencer->tryNext()));
    }
 
    EXPECT_THROW(this->m_sequencer->tryNext(), InsufficientCapacityException);
}

TYPED_TEST(SequencerTests, ShouldClaimSpecificSequence)
{
    std::int64_t sequence = 14;

    this->m_sequencer->claim(sequence);
    this->m_sequencer->publish(sequence);

    EXPECT_EQ(this->m_sequencer->next(), sequence + 1);
}

TYPED_TEST(SequencerTests, ShouldNotAllowBulkNextLessThanZero)
{
    EXPECT_THROW(this->m_sequencer->next(-1), ArgumentException);
}

TYPED_TEST(SequencerTests, ShouldNotAllowBulkNextOfZero)
{
    EXPECT_THROW(this->m_sequencer->next(0), ArgumentException);
}

TYPED_TEST(SequencerTests, ShouldNotAllowBulkTryNextLessThanZero)
{
    EXPECT_THROW(this->m_sequencer->tryNext(-1), ArgumentException);
}

TYPED_TEST(SequencerTests, ShouldNotAllowBulkTryNextOfZero)
{
    EXPECT_THROW(this->m_sequencer->tryNext(0), ArgumentException);
}

// Test suite end - handled by Google Test TYPED_TEST_SUITE
