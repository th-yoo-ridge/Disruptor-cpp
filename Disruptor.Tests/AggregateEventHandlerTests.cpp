#include "stdafx.h"

#include "Disruptor/AggregateEventHandler.h"

#include "AggregateEventHandlerTestsFixture.h"
#include "SequencerFixture.h"


using namespace Disruptor;
using namespace Disruptor::Tests;



TEST_F(AggregateEventHandlerTestsFixture, ShouldCallOnEventInSequence)
{
    auto evt = 7;
    const std::int64_t sequence = 3;
    const auto endOfBatch = true;

    EXPECT_CALL(*m_eventHandlerMock1, onEvent(evt, sequence, endOfBatch)).Times(1);
    EXPECT_CALL(*m_eventHandlerMock2, onEvent(evt, sequence, endOfBatch)).Times(1);
    EXPECT_CALL(*m_eventHandlerMock3, onEvent(evt, sequence, endOfBatch)).Times(1);

    m_aggregateEventHandler->onEvent(evt, sequence, endOfBatch);
}

TEST_F(AggregateEventHandlerTestsFixture, ShouldCallOnStartInSequence)
{
    EXPECT_CALL(*m_eventHandlerMock1, onStart()).Times(1);
    EXPECT_CALL(*m_eventHandlerMock2, onStart()).Times(1);
    EXPECT_CALL(*m_eventHandlerMock3, onStart()).Times(1);

    m_aggregateEventHandler->onStart();
}

TEST_F(AggregateEventHandlerTestsFixture, ShouldCallOnShutdownInSequence)
{
    EXPECT_CALL(*m_eventHandlerMock1, onShutdown()).Times(1);
    EXPECT_CALL(*m_eventHandlerMock2, onShutdown()).Times(1);
    EXPECT_CALL(*m_eventHandlerMock3, onShutdown()).Times(1);

    m_aggregateEventHandler->onShutdown();
}

TEST_F(AggregateEventHandlerTestsFixture, ShouldHandleEmptyListOfEventHandlers)
{
    auto newAggregateEventHandler = std::make_shared< AggregateEventHandler< std::int32_t > >();

    auto v = 7;
    EXPECT_NO_THROW(newAggregateEventHandler->onEvent(v, 0, true));
    EXPECT_NO_THROW(newAggregateEventHandler->onStart());
    EXPECT_NO_THROW(newAggregateEventHandler->onShutdown());
}

