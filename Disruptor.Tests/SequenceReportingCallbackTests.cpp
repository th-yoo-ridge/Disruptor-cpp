#include "stdafx.h"
#include <gtest/gtest.h>

#include "Disruptor/BatchEventProcessor.h"
#include "Disruptor/ISequenceReportingEventHandler.h"
#include "Disruptor/RingBuffer.h"

#include "Disruptor.TestTools/ManualResetEvent.h"

#include "StubEvent.h"


namespace Disruptor
{
namespace Tests
{

    class SequenceReportingCallbackTests : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_callbackSignal = std::make_shared< ManualResetEvent >(false);
            m_onEndOfBatchSignal = std::make_shared< ManualResetEvent >(false);
        }

        std::shared_ptr< ManualResetEvent > m_callbackSignal;
        std::shared_ptr< ManualResetEvent > m_onEndOfBatchSignal;
    };
    
    class TestSequenceReportingEventHandler : public ISequenceReportingEventHandler< StubEvent >
    {
    public:
        TestSequenceReportingEventHandler(const std::shared_ptr< ManualResetEvent >& callbackSignal,
                                          const std::shared_ptr< ManualResetEvent >& onEndOfBatchSignal)
            : m_callbackSignal(callbackSignal)
            , m_onEndOfBatchSignal(onEndOfBatchSignal)
        {
        }

        void setSequenceCallback(const std::shared_ptr< ISequence >& sequenceTrackerCallback) override
        {
            m_sequenceCallback = sequenceTrackerCallback;
        }

        void onEvent(StubEvent& /*evt*/, std::int64_t sequence, bool endOfBatch) override
        {
            m_sequenceCallback->setValue(sequence);
            m_callbackSignal->set();

            if (endOfBatch)
            {
                m_onEndOfBatchSignal->waitOne();
            }
        }

    private:
        std::shared_ptr< ISequence > m_sequenceCallback;
        std::shared_ptr< ManualResetEvent > m_callbackSignal;
        std::shared_ptr< ManualResetEvent > m_onEndOfBatchSignal;
    };

} // namespace Tests
} // namespace Disruptor


using namespace Disruptor;
using namespace Disruptor::Tests;


TEST_F(SequenceReportingCallbackTests, ShouldReportProgressByUpdatingSequenceViaCallback)
{
    auto ringBuffer = RingBuffer< StubEvent >::createMultiProducer([] { return StubEvent(-1); }, 16);
    auto sequenceBarrier = ringBuffer->newBarrier();
    auto handler = std::make_shared< TestSequenceReportingEventHandler >(m_callbackSignal, m_onEndOfBatchSignal);
    auto batchEventProcessor = std::make_shared< BatchEventProcessor< StubEvent > >(ringBuffer, sequenceBarrier, handler);
    ringBuffer->addGatingSequences({ batchEventProcessor->sequence() });

    std::thread thread([&] { batchEventProcessor->run(); });

    EXPECT_EQ(-1L, batchEventProcessor->sequence()->value());
    ringBuffer->publish(ringBuffer->next());

    m_callbackSignal->waitOne();
    EXPECT_EQ(0L, batchEventProcessor->sequence()->value());

    m_onEndOfBatchSignal->set();
    EXPECT_EQ(0L, batchEventProcessor->sequence()->value());

    batchEventProcessor->halt();

    if (thread.joinable())
        thread.join();
}
