#pragma once

#include <atomic>
#include <cstdint>
#include <barrier>
#include <memory>

#include "Disruptor/IWorkHandler.h"
#include "Disruptor/ILifecycleAware.h"

#include "Disruptor.Tests/TestEvent.h"


namespace Disruptor
{
namespace Tests
{

    class TestWorkHandler : public IWorkHandler< TestEvent >, public ILifecycleAware
    {
    public:
        TestWorkHandler() 
            : m_readyToProcessEvent(0)
            , m_stopped(false)
            , m_barrier(std::make_shared< std::barrier<> >(2))
        {}

        void onEvent(TestEvent& evt) override;

        void processEvent();

        void stopWaiting();

        void onStart() override;

        void onShutdown() override;

        void awaitStart();

    private:
        void waitForAndSetFlag(std::int32_t newValue);

        std::atomic< std::int32_t > m_readyToProcessEvent;
        std::atomic< bool > m_stopped;
        std::shared_ptr< std::barrier<> > m_barrier;
    };

} // namespace Tests
} // namespace Disruptor
