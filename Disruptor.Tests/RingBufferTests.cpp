#include "stdafx.h"

#include "Disruptor.TestTools/ManualResetEvent.h"

#include "RingBufferTestsFixture.h"


using namespace Disruptor;
using namespace Disruptor::Tests;

TEST_F(RingBufferTestsFixture, ShouldClaimAndGet)
{
    EXPECT_EQ(Sequence::InitialCursorValue, m_ringBuffer->cursor());
    
    auto expectedEvent = StubEvent(2701);

    auto claimSequence = m_ringBuffer->next();
    auto& oldEvent = (*m_ringBuffer)[claimSequence];
    oldEvent.copy(expectedEvent);
    m_ringBuffer->publish(claimSequence);

    auto sequence = m_sequenceBarrier->waitFor(0);
    EXPECT_EQ(0, sequence);

    auto& evt = (*m_ringBuffer)[sequence];
    EXPECT_EQ(expectedEvent, evt);
    
    EXPECT_EQ(0L, m_ringBuffer->cursor());
}

TEST_F(RingBufferTestsFixture, ShouldClaimAndGetInSeparateThread)
{
    auto events = getEvents(0, 0);

    auto expectedEvent = StubEvent(2701);

    auto sequence = m_ringBuffer->next();
    auto& oldEvent = (*m_ringBuffer)[sequence];
    oldEvent.copy(expectedEvent);
    m_ringBuffer->publishEvent(StubEvent::translator(), expectedEvent.value(), expectedEvent.testString());

    EXPECT_EQ(expectedEvent, events.get()[0]);
}

TEST_F(RingBufferTestsFixture, ShouldClaimAndGetMultipleMessages)
{
    auto numEvents = m_ringBuffer->bufferSize();
    for (auto i = 0; i < numEvents; ++i)
    {
        m_ringBuffer->publishEvent(StubEvent::translator(), i, std::string());
    }

    auto expectedSequence = numEvents - 1;
    auto available = m_sequenceBarrier->waitFor(expectedSequence);
    EXPECT_EQ(expectedSequence, available);

    for (auto i = 0; i < numEvents; ++i)
    {
        EXPECT_EQ(i, (*m_ringBuffer)[i].value());
    }
}

TEST_F(RingBufferTestsFixture, ShouldWrap)
{
    auto numEvents = m_ringBuffer->bufferSize();
    auto offset = 1000;
    for (auto i = 0; i < numEvents + offset; ++i)
    {
        m_ringBuffer->publishEvent(StubEvent::translator(), i, std::string());
    }

    auto expectedSequence = numEvents + offset - 1;
    auto available = m_sequenceBarrier->waitFor(expectedSequence);
    EXPECT_EQ(expectedSequence, available);

    for (auto i = offset; i < numEvents + offset; ++i)
    {
        EXPECT_EQ(i, (*m_ringBuffer)[i].value());
    }
}

TEST_F(RingBufferTestsFixture, ShouldPreventWrapping)
{
    auto sequence = std::make_shared< Sequence >(Sequence::InitialCursorValue);
    auto ringBuffer = RingBuffer< StubEvent >::createMultiProducer([] { return StubEvent(-1); }, 4);
    ringBuffer->addGatingSequences({ sequence });

    ringBuffer->publishEvent(StubEvent::translator(), 0, std::string("0"));
    ringBuffer->publishEvent(StubEvent::translator(), 1, std::string("1"));
    ringBuffer->publishEvent(StubEvent::translator(), 2, std::string("2"));
    ringBuffer->publishEvent(StubEvent::translator(), 3, std::string("3"));

    EXPECT_FALSE(ringBuffer->tryPublishEvent(StubEvent::translator(), 3, std::string("3")));
}

TEST_F(RingBufferTestsFixture, ShouldThrowExceptionIfBufferIsFull)
{
    m_ringBuffer->addGatingSequences({ std::make_shared< Sequence >(m_ringBuffer->bufferSize()) });

    try
    {
        for (auto i = 0; i < m_ringBuffer->bufferSize(); ++i)
        {
            m_ringBuffer->publish(m_ringBuffer->tryNext());
        }
    }
    catch (std::exception&)
    {
        throw std::logic_error("Should not have thrown exception");
    }

    try
    {
        m_ringBuffer->tryNext();
        throw std::logic_error("Exception should have been thrown");
    }
    catch (InsufficientCapacityException&)
    {
    }
}

TEST_F(RingBufferTestsFixture, ShouldPreventProducersOvertakingEventProcessorsWrapPoint)
{
    auto ringBufferSize = 4;
    ManualResetEvent mre(false);
    auto producerComplete = false;
    auto ringBuffer = std::make_shared< RingBuffer< StubEvent > >([] { return StubEvent(-1); }, ringBufferSize);
    auto processor = std::make_shared< TestEventProcessor >(ringBuffer->newBarrier());
    ringBuffer->addGatingSequences({ processor->sequence() });

    std::thread thread([&]
    {
        for (auto i = 0; i <= ringBufferSize; i++) // produce 5 events
        {
            auto sequence = ringBuffer->next();
            auto& evt = (*ringBuffer)[sequence];
            evt.value(i);
            ringBuffer->publish(sequence);
        
            if (i == 3) // unblock main thread after 4th eventData published
            {
                mre.set();
            }
        }
        
        producerComplete = true;
    });

    mre.waitOne();

    EXPECT_EQ(ringBuffer->cursor(), ringBufferSize - 1);
    EXPECT_FALSE(producerComplete);

    processor->run();

    if (thread.joinable())
        thread.join();

    EXPECT_TRUE(producerComplete);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEvent)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();

    ringBuffer->publishEvent(translator);
    ringBuffer->tryPublishEvent(translator);

    EXPECT_EQ(m_ringBufferWithEvents(0, 1), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventOneArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();
    
    ringBuffer->publishEvent(translator, std::string("Foo"));
    ringBuffer->tryPublishEvent(translator, std::string("Foo"));

    EXPECT_EQ(m_ringBufferWithEvents(std::string("Foo-0"), std::string("Foo-1")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventTwoArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    ringBuffer->publishEvent(translator, std::string("Foo"), std::string("Bar"));
    ringBuffer->tryPublishEvent(translator, std::string("Foo"), std::string("Bar"));

    EXPECT_EQ(m_ringBufferWithEvents(std::string("FooBar-0"), std::string("FooBar-1")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventThreeArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    ringBuffer->publishEvent(translator, std::string("Foo"), std::string("Bar"), std::string("Baz"));
    ringBuffer->tryPublishEvent(translator, std::string("Foo"), std::string("Bar"), std::string("Baz"));

    EXPECT_EQ(m_ringBufferWithEvents(std::string("FooBarBaz-0"), std::string("FooBarBaz-1")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEvents)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto eventTranslator = std::make_shared< NoArgEventTranslator >();
    auto translators = { eventTranslator, eventTranslator };

    ringBuffer->publishEvents(translators);
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translators));

    EXPECT_EQ(m_ringBufferWithEvents4(0, 1, 2, 3), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsIfBatchIsLargerThanRingBuffer)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto eventTranslator = std::make_shared< NoArgEventTranslator >();
    auto translators = { eventTranslator, eventTranslator, eventTranslator, eventTranslator, eventTranslator };

    EXPECT_THROW(ringBuffer->tryPublishEvents(translators), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsWithBatchSizeOfOne)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto eventTranslator = std::make_shared< NoArgEventTranslator >();
    auto translators = { eventTranslator, eventTranslator, eventTranslator };

    ringBuffer->publishEvents(translators, 0, 1);
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translators, 0, 1));

    EXPECT_EQ(m_ringBufferWithEvents4(0, 1, std::any(), std::any()), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsWithinBatch)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto eventTranslator = std::make_shared< NoArgEventTranslator >();
    auto translators = { eventTranslator, eventTranslator, eventTranslator };

    ringBuffer->publishEvents(translators, 1, 2);
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translators, 1, 2));

    EXPECT_EQ(m_ringBufferWithEvents4(0, 1, 2, 3), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsOneArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    ringBuffer->publishEvents(translator, { std::string("Foo"), std::string("Foo") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, { std::string("Foo"), std::string("Foo") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo-0"), std::string("Foo-1"), std::string("Foo-2"), std::string("Foo-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsOneArgIfBatchIsLargerThanRingBuffer)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, { std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsOneArgBatchSizeOfOne)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    ringBuffer->publishEvents(translator, 0, 1, { std::string("Foo"), std::string("Foo") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 0, 1, { std::string("Foo"), std::string("Foo") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo-0"), std::string("Foo-1"), std::any(), std::any()), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsOneArgWithinBatch)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    ringBuffer->publishEvents(translator, 1, 2, { std::string("Foo"), std::string("Foo"), std::string("Foo") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 1, 2, { std::string("Foo"), std::string("Foo"), std::string("Foo") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo-0"), std::string("Foo-1"), std::string("Foo-2"), std::string("Foo-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsTwoArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    ringBuffer->publishEvents(translator, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("FooBar-0"), std::string("FooBar-1"), std::string("FooBar-2"), std::string("FooBar-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsITwoArgIfBatchSizeIsBiggerThanRingBuffer)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents
    (
        translator,
        { std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo") },
        { std::string("Bar"), std::string("Bar"), std::string("Bar"), std::string("Bar"), std::string("Bar") }
    ), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsTwoArgWithBatchSizeOfOne)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    ringBuffer->publishEvents(translator, 0, 1, { std::string("Foo0"), std::string("Foo1") }, { std::string("Bar0"), std::string("Bar1") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 0, 1, { std::string("Foo2"), std::string("Foo3") }, { std::string("Bar2"), std::string("Bar3") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo0Bar0-0"), std::string("Foo2Bar2-1"), std::any(), std::any()), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsTwoArgWithinBatch)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    ringBuffer->publishEvents(translator, 1, 2, { std::string("Foo0"), std::string("Foo1"), std::string("Foo2") }, { std::string("Bar0"), std::string("Bar1"), std::string("Bar2") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 1, 2, { std::string("Foo3"), std::string("Foo4"), std::string("Foo5") }, { std::string("Bar3"), std::string("Bar4"), std::string("Bar5") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo1Bar1-0"), std::string("Foo2Bar2-1"), std::string("Foo4Bar4-2"), std::string("Foo5Bar5-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsThreeArg)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    ringBuffer->publishEvents(translator, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }, { std::string("Baz"), std::string("Baz") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }, { std::string("Baz"), std::string("Baz") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("FooBarBaz-0"), std::string("FooBarBaz-1"), std::string("FooBarBaz-2"), std::string("FooBarBaz-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsThreeArgIfBatchIsLargerThanRingBuffer)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents
    (
        translator,
        { std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo"), std::string("Foo") },
        { std::string("Bar"), std::string("Bar"), std::string("Bar"), std::string("Bar"), std::string("Bar") },
        { std::string("Baz"), std::string("Baz"), std::string("Baz"), std::string("Baz"), std::string("Baz") }
    ), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsThreeArgBatchSizeOfOne)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    ringBuffer->publishEvents(translator, 0, 1, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }, { std::string("Baz"), std::string("Baz") });
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 0, 1, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }, { std::string("Baz"), std::string("Baz") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("FooBarBaz-0"), std::string("FooBarBaz-1"), std::any(), std::any()), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldPublishEventsThreeArgWithinBatch)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    ringBuffer->publishEvents(translator, 1, 2, { std::string("Foo0"), std::string("Foo1"), std::string("Foo2") },
                                                { std::string("Bar0"), std::string("Bar1"), std::string("Bar2") },
                                                { std::string("Baz0"), std::string("Baz1"), std::string("Baz2") });
    
    EXPECT_TRUE(ringBuffer->tryPublishEvents(translator, 1, 2, { std::string("Foo3"), std::string("Foo4"), std::string("Foo5") },
                                                                     { std::string("Bar3"), std::string("Bar4"), std::string("Bar5") },
                                                                     { std::string("Baz3"), std::string("Baz4"), std::string("Baz5") }));

    EXPECT_EQ(m_ringBufferWithEvents4(std::string("Foo1Bar1Baz1-0"), std::string("Foo2Bar2Baz2-1"), std::string("Foo4Bar4Baz4-2"), std::string("Foo5Bar5Baz5-3")), *ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsWhenBatchSizeIs0)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator, translator };

    EXPECT_THROW(ringBuffer->publishEvents(translators, 1, 0), ArgumentException);
    EXPECT_THROW(ringBuffer->tryPublishEvents(translators, 1, 0), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator };

    EXPECT_THROW(ringBuffer->publishEvents(translators, 1, 3), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator };

    EXPECT_THROW(ringBuffer->tryPublishEvents(translators, 1, 3), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator, translator };

    EXPECT_THROW(ringBuffer->publishEvents(translators, 1, -1), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator, translator };

    EXPECT_THROW(ringBuffer->tryPublishEvents(translators, 1, -1), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator, translator };

    EXPECT_THROW(ringBuffer->publishEvents(translators, -1, 2), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< NoArgEventTranslator >();
    auto translators = { translator, translator, translator, translator };

    EXPECT_THROW(ringBuffer->tryPublishEvents(translators, -1, 2), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsOneArgWhenBatchSizeIs0)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") }), ArgumentException);
    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsOneArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsOneArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsOneArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsOneArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsOneArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsOneArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< OneArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsTwoArgWhenBatchSizeIs0)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);
    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsTwoArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsTwoArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsTwoArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsTwoArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsTwoArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsTwoArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< TwoArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") }, { std::string("Bar"), std::string("Bar") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsThreeArgWhenBatchSizeIs0)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") },
                                                                  { std::string("Bar"), std::string("Bar") },
                                                                  { std::string("Baz"), std::string("Baz") }), ArgumentException);

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 0, { std::string("Foo"), std::string("Foo") },
                                                                     { std::string("Bar"), std::string("Bar") },
                                                                     { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsThreeArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") },
                                                                  { std::string("Bar"), std::string("Bar") },
                                                                  { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsThreeArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") },
                                                                   { std::string("Bar"), std::string("Bar") },
                                                                   { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotPublishEventsThreeArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->publishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") },
                                                                   { std::string("Bar"), std::string("Bar") },
                                                                   { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsThreeArgWhenBatchExtendsPastEndOfArray)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, 3, { std::string("Foo"), std::string("Foo") },
                                                                     { std::string("Bar"), std::string("Bar") },
                                                                     { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsThreeArgWhenBatchSizeIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, 1, -1, { std::string("Foo"), std::string("Foo") },
                                                                      { std::string("Bar"), std::string("Bar") },
                                                                      { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldNotTryPublishEventsThreeArgWhenBatchStartsAtIsNegative)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 4);
    auto translator = std::make_shared< ThreeArgEventTranslator >();

    EXPECT_THROW(ringBuffer->tryPublishEvents(translator, -1, 2, { std::string("Foo"), std::string("Foo") },
                                                                      { std::string("Bar"), std::string("Bar") },
                                                                      { std::string("Baz"), std::string("Baz") }), ArgumentException);

    assertEmptyRingBuffer(*ringBuffer);
}

TEST_F(RingBufferTestsFixture, ShouldAddAndRemoveSequences)
{
    auto ringBuffer = RingBuffer< std::any >::createSingleProducer([] { return std::any(); }, 16);
    auto sequenceThree = std::make_shared< Sequence >(-1);
    auto sequenceSeven = std::make_shared< Sequence >(-1);

    ringBuffer->addGatingSequences({ sequenceThree, sequenceSeven });

    for (auto i = 0; i < 10; i++)
    {
        ringBuffer->publish(ringBuffer->next());
    }
    
    sequenceThree->setValue(3);
    sequenceSeven->setValue(7);
    
    EXPECT_EQ(ringBuffer->getMinimumGatingSequence(), 3L);
    EXPECT_TRUE(ringBuffer->removeGatingSequence(sequenceThree));
    EXPECT_EQ(ringBuffer->getMinimumGatingSequence(), 7L);
}

TEST_F(RingBufferTestsFixture, ShouldHandleResetToAndNotWrapUnecessarilySingleProducer)
{
    assertHandleResetAndNotWrap(RingBuffer< StubEvent >::createSingleProducer(StubEvent::eventFactory(), 4));
}

TEST_F(RingBufferTestsFixture, ShouldHandleResetToAndNotWrapUnecessarilyMultiProducer)
{
    assertHandleResetAndNotWrap(RingBuffer< StubEvent >::createMultiProducer(StubEvent::eventFactory(), 4));
}

