#pragma once

#include <cstdint>
#include <future>
#include <memory>
#include <ostream>
#include <vector>

#include <any>
#include <boost/test/unit_test.hpp>

#include "Disruptor/IEventTranslator.h"
#include "Disruptor/IEventTranslatorVararg.h"
#include "Disruptor/NoOpEventProcessor.h"
#include "Disruptor/RingBuffer.h"

#include "Disruptor.Tests/StubEvent.h"
#include "Disruptor.Tests/TestWaiter.h"


namespace Disruptor
{
namespace Tests
{

    struct RingBufferTestsFixture
    {
        class NoArgEventTranslator;
        class OneArgEventTranslator;
        class TestEventProcessor;
        class ThreeArgEventTranslator;
        class TwoArgEventTranslator;

        RingBufferTestsFixture();

        std::future< std::vector< StubEvent > > getEvents(std::int64_t initial, std::int64_t toWaitFor);

        void assertHandleResetAndNotWrap(const std::shared_ptr< RingBuffer< StubEvent > >& rb);

        static void assertEmptyRingBuffer(const RingBuffer< std::any >& ringBuffer);


        class RingBufferEquals
        {
        public:
            RingBufferEquals(const std::initializer_list< std::any >& values);

            bool operator==(const RingBuffer< std::any >& ringBuffer) const;

            void writeDescriptionTo(std::ostream& stream) const;

        private:
            // http://stackoverflow.com/questions/6029092/compare-boostany-contents
            static bool anyEqual(const std::any& lhs, const std::any& rhs);

            static std::string anyToString(const std::any& value);

            std::vector< std::any > m_values;
        };


        std::shared_ptr< RingBuffer< StubEvent > > m_ringBuffer;
        std::shared_ptr< ISequenceBarrier > m_sequenceBarrier;

        std::function< RingBufferEquals(const std::any&, const std::any&) > m_ringBufferWithEvents =
            [](const std::any& l1, const std::any& l2) { return RingBufferEquals({ l1, l2 }); };

        std::function< RingBufferEquals(const std::any&, const std::any&, const std::any&, const std::any&) > m_ringBufferWithEvents4 =
            [](const std::any& l1, const std::any& l2, const std::any& l3, const std::any& l4) { return RingBufferEquals({ l1, l2, l3, l4 }); };
    };


    class RingBufferTestsFixture::NoArgEventTranslator : public IEventTranslator< std::any >
    {
    public:
        void translateTo(std::any& eventData, std::int64_t sequence) override;
    };


    class RingBufferTestsFixture::ThreeArgEventTranslator : public IEventTranslatorVararg< std::any, std::string, std::string, std::string >
    {
    public:
        void translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0, const std::string& arg1, const std::string& arg2) override;
    };
    
    
    class RingBufferTestsFixture::TwoArgEventTranslator : public IEventTranslatorVararg< std::any, std::string, std::string >
    {
    public:
        void translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0, const std::string& arg1) override;
    };
    
    
    class RingBufferTestsFixture::OneArgEventTranslator : public IEventTranslatorVararg< std::any, std::string >
    {
    public:
        void translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0) override;
    };
    
    
    class RingBufferTestsFixture::TestEventProcessor : public IEventProcessor
    {
    public:
        explicit TestEventProcessor(const std::shared_ptr< ISequenceBarrier >& sequenceBarrier);

        std::shared_ptr< ISequence > sequence() const override;

        void halt() override;

        void run() override;

        bool isRunning() const override;

    private:
        std::shared_ptr< ISequenceBarrier > m_sequenceBarrier;
        std::shared_ptr< ISequence > m_sequence;
        bool m_isRunning = false;
    };
        
} // namespace Tests
} // namespace Disruptor


namespace std
{
    ostream& operator<<(ostream& stream, const Disruptor::Tests::RingBufferTestsFixture::RingBufferEquals& equals);

} // namespace std
