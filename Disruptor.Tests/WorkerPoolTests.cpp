#include "stdafx.h"

#include "Disruptor/BasicExecutor.h"
#include "Disruptor/FatalExceptionHandler.h"
#include "Disruptor/RoundRobinThreadAffinedTaskScheduler.h"
#include "Disruptor/WorkerPool.h"


namespace Disruptor
{
namespace Tests
{
    
    class AtomicLong
    {
    public:
        AtomicLong() : m_value(0)
        {}

        AtomicLong(const AtomicLong& other)
            : m_value(other.value())
        {}

        void operator=(const AtomicLong& other)
        {
            m_value = other.value();
        }

        std::int64_t value() const
        {
            return m_value;
        }

        void increment()
        {
            ++m_value;
        }

    private:
        std::atomic< std::int64_t > m_value;
    };

    class AtomicLongWorkHandler : public IWorkHandler< AtomicLong >
    {
    public:
        void onEvent(AtomicLong& evt) override
        {
            evt.increment();
        }
    };

} // namespace Tests
} // namespace Disruptor


using namespace Disruptor;
using namespace Disruptor::Tests;


class WorkerPoolTests : public ::testing::Test
{
};

TEST_F(WorkerPoolTests, ShouldProcessEachMessageByOnlyOneWorker)
{
    auto taskScheduler = std::make_shared< RoundRobinThreadAffinedTaskScheduler >();
    taskScheduler->start(2);

    WorkerPool< AtomicLong > pool
    (
        [] { return AtomicLong(); },
        std::make_shared< FatalExceptionHandler< AtomicLong > >(),
        { std::make_shared< AtomicLongWorkHandler >(), std::make_shared< AtomicLongWorkHandler >() }
    );

    auto ringBuffer = pool.start(std::make_shared< BasicExecutor >(taskScheduler));

    ringBuffer->next();
    ringBuffer->next();
    ringBuffer->publish(0);
    ringBuffer->publish(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ((*ringBuffer)[0].value(), 1L);
    EXPECT_EQ((*ringBuffer)[1].value(), 1L);
    
    pool.drainAndHalt();
    taskScheduler->stop();
}

TEST_F(WorkerPoolTests, ShouldProcessOnlyOnceItHasBeenPublished)
{
    auto taskScheduler = std::make_shared< RoundRobinThreadAffinedTaskScheduler >();
    taskScheduler->start(2);

    WorkerPool< AtomicLong > pool
    (
        [] { return AtomicLong(); },
        std::make_shared< FatalExceptionHandler< AtomicLong > >(),
        { std::make_shared< AtomicLongWorkHandler >(), std::make_shared< AtomicLongWorkHandler >() }
    );

    auto ringBuffer = pool.start(std::make_shared< BasicExecutor >(taskScheduler));

    auto seq1 = ringBuffer->next();
    auto seq2 = ringBuffer->next();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    EXPECT_EQ((*ringBuffer)[seq1].value(), 0L);
    EXPECT_EQ((*ringBuffer)[seq2].value(), 0L);
    
    pool.halt();
    taskScheduler->stop();
}

