#include "stdafx.h"
#include "RingBufferTestsFixture.h"


namespace Disruptor
{
namespace Tests
{

    void RingBufferTestsFixture::SetUp()
    {
        m_ringBuffer = RingBuffer< StubEvent >::createMultiProducer([] { return StubEvent(-1); }, 32);
        m_sequenceBarrier = m_ringBuffer->newBarrier();

        auto processor = std::make_shared< NoOpEventProcessor< StubEvent > >(m_ringBuffer);
        m_ringBuffer->addGatingSequences({processor->sequence()});
    }

    std::future< std::vector< StubEvent > > RingBufferTestsFixture::getEvents(std::int64_t initial, std::int64_t toWaitFor)
    {
        auto barrier = std::make_shared< std::barrier<> >(2);
        auto dependencyBarrier = m_ringBuffer->newBarrier();

        auto testWaiter = std::make_shared< TestWaiter >(barrier, dependencyBarrier, m_ringBuffer, initial, toWaitFor);
        auto task = std::async(std::launch::async, [=] { return testWaiter->call(); });

        barrier->arrive_and_wait();

        return task;
    }

    void RingBufferTestsFixture::assertHandleResetAndNotWrap(const std::shared_ptr< RingBuffer< StubEvent > >& rb)
    {
        auto sequence = std::make_shared< Sequence >();
        rb->addGatingSequences({sequence});

        for (auto i = 0; i < 128; ++i)
        {
            rb->publish(rb->next());
            sequence->incrementAndGet();
        }

        EXPECT_EQ(rb->cursor(), 127L);

        rb->resetTo(31);
        sequence->setValue(31);

        for (auto i = 0; i < 4; ++i)
        {
            rb->publish(rb->next());
        }

        EXPECT_FALSE(rb->hasAvailableCapacity(1));
    }

    void RingBufferTestsFixture::assertEmptyRingBuffer(const RingBuffer< std::any >& ringBuffer)
    {
        EXPECT_FALSE(ringBuffer[0].has_value());
        EXPECT_FALSE(ringBuffer[1].has_value());
        EXPECT_FALSE(ringBuffer[2].has_value());
        EXPECT_FALSE(ringBuffer[3].has_value());
    }

    void RingBufferTestsFixture::NoArgEventTranslator::translateTo(std::any& eventData, std::int64_t sequence)
    {
        eventData = static_cast< int >(sequence);
    }

    void RingBufferTestsFixture::ThreeArgEventTranslator::translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0, const std::string& arg1, const std::string& arg2)
    {
        eventData = arg0 + arg1 + arg2 + "-" + std::to_string(sequence);
    }

    void RingBufferTestsFixture::TwoArgEventTranslator::translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0, const std::string& arg1)
    {
        eventData = arg0 + arg1 + "-" + std::to_string(sequence);
    }

    void RingBufferTestsFixture::OneArgEventTranslator::translateTo(std::any& eventData, std::int64_t sequence, const std::string& arg0)
    {
        eventData = arg0 + "-" + std::to_string(sequence);
    }

    RingBufferTestsFixture::TestEventProcessor::TestEventProcessor(const std::shared_ptr< ISequenceBarrier >& sequenceBarrier)
        : m_sequenceBarrier(sequenceBarrier)
        , m_sequence(std::make_shared< Sequence >())
    {
    }

    std::shared_ptr< ISequence > RingBufferTestsFixture::TestEventProcessor::sequence() const
    {
        return m_sequence;
    }

    void RingBufferTestsFixture::TestEventProcessor::halt()
    {
        m_isRunning = false;
    }

    void RingBufferTestsFixture::TestEventProcessor::run()
    {
        m_isRunning = true;
        m_sequenceBarrier->waitFor(0L);
        m_sequence->setValue(m_sequence->value() + 1);
    }

    bool RingBufferTestsFixture::TestEventProcessor::isRunning() const
    {
        return m_isRunning;
    }

    RingBufferTestsFixture::RingBufferEquals::RingBufferEquals(const std::initializer_list< std::any >& values)
        : m_values(values)
    {
    }

    bool RingBufferTestsFixture::RingBufferEquals::operator==(const RingBuffer< std::any >& ringBuffer) const
    {
        auto valid = true;
        for (auto i = 0u; i < m_values.size(); ++i)
        {
            auto value = m_values[i];
            //valid &= value.empty() || anyEqual(ringBuffer[i], value);
            valid &= !value.has_value() || anyEqual(ringBuffer[i], value);
        }
        return valid;
    }

    void RingBufferTestsFixture::RingBufferEquals::writeDescriptionTo(std::ostream& stream) const
    {
        auto firstItem = true;
        for (auto&& value : m_values)
        {
            if (firstItem)
                firstItem = false;
            else
                stream << ", ";

            stream << anyToString(value);
        }
    }

    bool RingBufferTestsFixture::RingBufferEquals::anyEqual(const std::any& lhs, const std::any& rhs)
    {
        if (!lhs.has_value() && !rhs.has_value())
            return true;

        if (!lhs.has_value() || !rhs.has_value())
            return false;

        if (lhs.type() != rhs.type())
            return false;

        if (lhs.type() == typeid(std::string))
            return std::any_cast< std::string >(lhs) == std::any_cast< std::string >(rhs);

        if (lhs.type() == typeid(int))
            return std::any_cast< int >(lhs) == std::any_cast< int >(rhs);

        throw std::runtime_error("comparison of any unimplemented for type");
    }

    std::string RingBufferTestsFixture::RingBufferEquals::anyToString(const std::any& value)
    {
        if (!value.has_value())
            return "";

        if (value.type() == typeid(std::string))
            return std::any_cast< std::string >(value);

        if (value.type() == typeid(int))
            return std::to_string(std::any_cast< int >(value));

        throw std::runtime_error("cannot get string representation of unimplemented any type");
    }

} // namespace Tests
} // namespace Disruptor


namespace std
{

    ostream& operator<<(ostream& stream, const Disruptor::Tests::RingBufferTestsFixture::RingBufferEquals& equals)
    {
        equals.writeDescriptionTo(stream);
        return stream;
    }

} // namespace std
