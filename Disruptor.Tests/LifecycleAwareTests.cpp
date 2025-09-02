#include "stdafx.h"

#include "Disruptor/BatchEventProcessor.h"
#include "Disruptor/IEventHandler.h"
#include "Disruptor/IgnoreExceptionHandler.h"
#include "Disruptor/ILifecycleAware.h"
#include "Disruptor/RingBuffer.h"

#include "Disruptor.TestTools/ManualResetEvent.h"

#include "StubEvent.h"
#include "StubExecutor.h"


using namespace Disruptor;
using namespace ::Disruptor::Tests;


class LifecycleAwareTests : public ::testing::Test
{
public:

class LifecycleAwareEventHandler : public IEventHandler< StubEvent >, public ILifecycleAware
{
public:

    std::int32_t startCounter() const { return m_startCounter; }
    std::int32_t shutdownCounter() const { return m_shutdownCounter; }

    LifecycleAwareEventHandler(ManualResetEvent& startSignal, ManualResetEvent& shutdownSignal)
        : m_startSignal(startSignal)
        , m_shutdownSignal(shutdownSignal)
        , m_startCounter(0)
        , m_shutdownCounter(0)
    {}

    void onEvent(StubEvent&, std::int64_t, bool) override
    {}

    void onStart() override
    {
        ++m_startCounter;
        m_startSignal.set();
    }

    void onShutdown() override
    {
        ++m_shutdownCounter;
        m_shutdownSignal.set();
    }

private:
    ManualResetEvent& m_startSignal;
    ManualResetEvent& m_shutdownSignal;
    std::int32_t m_startCounter;
    std::int32_t m_shutdownCounter;
};

};

TEST_F(LifecycleAwareTests, ShouldNotifyOfBatchProcessorLifecycle)
{
    ManualResetEvent startSignal(false);
    ManualResetEvent shutdownSignal(false);
    auto ringBuffer = std::make_shared< RingBuffer< StubEvent > >([] { return StubEvent(0); }, 16);

    auto sequenceBarrier = ringBuffer->newBarrier();
    auto eventHandler = std::make_shared< LifecycleAwareEventHandler >(startSignal, shutdownSignal);
    auto batchEventProcessor = std::make_shared< BatchEventProcessor< StubEvent > >(ringBuffer, sequenceBarrier, eventHandler);

    auto thread = std::thread([&] { batchEventProcessor->run(); });

    startSignal.waitOne();
    batchEventProcessor->halt();

    shutdownSignal.waitOne();

    thread.join();

    EXPECT_EQ(eventHandler->startCounter(), 1);
    EXPECT_EQ(eventHandler->shutdownCounter(), 1);
}

