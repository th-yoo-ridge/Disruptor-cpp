#include "stdafx.h"

#include "BatchEventProcessorTestsFixture.h"


using namespace Disruptor;
using namespace Disruptor::Tests;



TEST_F(BatchEventProcessorTestsFixture, ShouldThrowExceptionOnSettingNullExceptionHandler)
{
    EXPECT_THROW(m_batchEventProcessor->setExceptionHandler(nullptr), ArgumentNullException);
}

TEST_F(BatchEventProcessorTestsFixture, ShouldCallMethodsInLifecycleOrder)
{
    EXPECT_CALL(*m_batchHandlerMock, onEvent((*m_ringBuffer)[0], 0, true)).WillOnce(testing::Invoke([this](StubEvent&, std::int64_t, bool)
    {
        m_countDownEvent.signal();
    }));

    std::thread thread([this] { m_batchEventProcessor->run(); });

    EXPECT_EQ(-1, m_batchEventProcessor->sequence()->value());

    m_ringBuffer->publish(m_ringBuffer->next());

    EXPECT_EQ(m_countDownEvent.wait(std::chrono::milliseconds(50)), true);
    m_batchEventProcessor->halt();
    thread.join();
}

TEST_F(BatchEventProcessorTestsFixture, ShouldCallMethodsInLifecycleOrderForBatch)
{
    EXPECT_CALL(*m_batchHandlerMock, onEvent((*m_ringBuffer)[0], 0, false)).Times(1);
    EXPECT_CALL(*m_batchHandlerMock, onEvent((*m_ringBuffer)[1], 1, false)).Times(1);
    EXPECT_CALL(*m_batchHandlerMock, onEvent((*m_ringBuffer)[2], 2, true)).WillOnce(testing::Invoke([this](StubEvent&, std::int64_t, bool)
    {
        m_countDownEvent.signal();
    }));

    m_ringBuffer->publish(m_ringBuffer->next());
    m_ringBuffer->publish(m_ringBuffer->next());
    m_ringBuffer->publish(m_ringBuffer->next());

    std::thread thread([this] { m_batchEventProcessor->run(); });

    EXPECT_EQ(m_countDownEvent.wait(std::chrono::milliseconds(50)), true);
    m_batchEventProcessor->halt();
    thread.join();
}

TEST_F(BatchEventProcessorTestsFixture, ShouldCallExceptionHandlerOnUncaughtException)
{
    InvalidOperationException ex("BatchEventProcessorTests.ShouldCallExceptionHandlerOnUncaughtException");
    m_batchEventProcessor->setExceptionHandler(m_excpetionHandlerMock);
    
    EXPECT_CALL(*m_batchHandlerMock, onEvent((*m_ringBuffer)[0], 0, true)).WillRepeatedly(testing::Throw(ex));

    EXPECT_CALL(*m_excpetionHandlerMock, handleEventException(testing::_, 0, (*m_ringBuffer)[0]))
        .WillRepeatedly(testing::Invoke([&](const std::exception& lException, std::int64_t, StubEvent&)
        {
            EXPECT_STREQ(lException.what(), ex.what());
            m_countDownEvent.signal();
        }));

    std::thread thread([this] { m_batchEventProcessor->run(); });

    m_ringBuffer->publish(m_ringBuffer->next());

    EXPECT_EQ(m_countDownEvent.wait(std::chrono::milliseconds(50)), true);
    m_batchEventProcessor->halt();
    thread.join();
}
