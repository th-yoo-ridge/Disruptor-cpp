#pragma once

#include <cstdint>
#include <memory>

#include "Disruptor/AggregateEventHandler.h"

#include "Disruptor.Tests/LifecycleAwareEventHandlerMock.h"


namespace Disruptor
{
namespace Tests
{

    class AggregateEventHandlerTestsFixture : public ::testing::Test
    {
    protected:
        void SetUp() override;

        std::shared_ptr< LifecycleAwareEventHandlerMock< std::int32_t > > m_eventHandlerMock1;
        std::shared_ptr< LifecycleAwareEventHandlerMock< std::int32_t > > m_eventHandlerMock2;
        std::shared_ptr< LifecycleAwareEventHandlerMock< std::int32_t > > m_eventHandlerMock3;

        std::shared_ptr< AggregateEventHandler< std::int32_t > > m_aggregateEventHandler;
    };

} // namespace Tests
} // namespace Disruptor
