#include "stdafx.h"

#include "ConsumerRepositoryTestsFixture.h"


using namespace Disruptor;
using namespace Disruptor::Tests;


TEST_F(ConsumerRepositoryTestsFixture, ShouldGetBarrierByHandler)
{
    m_consumerRepository.add(m_eventProcessor1, m_handler1, m_barrier1);

    EXPECT_EQ(m_consumerRepository.getBarrierFor(m_handler1), m_barrier1);
}

TEST_F(ConsumerRepositoryTestsFixture, ShouldReturnNullForBarrierWhenHandlerIsNotRegistered)
{
    EXPECT_EQ(m_consumerRepository.getBarrierFor(m_handler1), nullptr);
}

TEST_F(ConsumerRepositoryTestsFixture, ShouldGetLastEventProcessorsInChain)
{
    m_consumerRepository.add(m_eventProcessor1, m_handler1, m_barrier1);
    m_consumerRepository.add(m_eventProcessor2, m_handler2, m_barrier2);

    m_consumerRepository.unMarkEventProcessorsAsEndOfChain({ m_eventProcessor2->sequence() });

    auto lastEventProcessorsInChain = m_consumerRepository.getLastSequenceInChain(true);

    EXPECT_EQ(lastEventProcessorsInChain.size(), 1u);
    EXPECT_EQ(lastEventProcessorsInChain[0], m_eventProcessor1->sequence());
}

TEST_F(ConsumerRepositoryTestsFixture, ShouldRetrieveEventProcessorForHandler)
{
    m_consumerRepository.add(m_eventProcessor1, m_handler1, m_barrier1);

    EXPECT_EQ(m_consumerRepository.getEventProcessorFor(m_handler1), m_eventProcessor1);
}

TEST_F(ConsumerRepositoryTestsFixture, ShouldThrowExceptionWhenHandlerIsNotRegistered)
{
    EXPECT_THROW(m_consumerRepository.getEventProcessorFor(std::shared_ptr< SleepingEventHandler >()), Disruptor::ArgumentException);
}

TEST_F(ConsumerRepositoryTestsFixture, ShouldIterateAllEventProcessors)
{
    m_consumerRepository.add(m_eventProcessor1, m_handler1, m_barrier1);
    m_consumerRepository.add(m_eventProcessor2, m_handler2, m_barrier2);

    auto seen1 = false;
    auto seen2 = false;
    
    for (auto&& testEntryEventProcessorInfo : m_consumerRepository)
    {
        auto eventProcessorInfo = std::dynamic_pointer_cast< EventProcessorInfo< TestEvent > >(testEntryEventProcessorInfo);
        if (!seen1 && eventProcessorInfo->eventProcessor() == m_eventProcessor1 && eventProcessorInfo->handler() == m_handler1)
        {
            seen1 = true;
        }
        else if (!seen2 && eventProcessorInfo->eventProcessor() == m_eventProcessor2 && eventProcessorInfo->handler() == m_handler2)
        {
            seen2 = true;
        }
        else
        {
            FAIL() << "Unexpected eventProcessor info";
        }
    }

    EXPECT_TRUE(seen1);
    EXPECT_TRUE(seen2);
}
