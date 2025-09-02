#include "stdafx.h"

#include "Disruptor/NoOpEventProcessor.h"
#include "Disruptor/RingBuffer.h"

#include "LongEvent.h"


namespace Disruptor
{
namespace Tests
{

    class EventPublisherFixture : public ::testing::Test
    {
    public:
        struct Translator : public IEventTranslator< LongEvent >
        {
            explicit Translator(EventPublisherFixture&)
            {
            }

            void translateTo(LongEvent& eventData, std::int64_t sequence) override
            {
                eventData.value = sequence + 29;
            }

        };

        void SetUp() override
        {
            m_ringBuffer = RingBuffer< LongEvent >::createMultiProducer([]() { return LongEvent(); }, m_bufferSize);
            m_translator = std::make_shared< Translator >(*this);
        }

        const std::int32_t m_bufferSize = 32;
        const std::int64_t m_valueAdd = 29L;
        std::shared_ptr< RingBuffer< LongEvent > > m_ringBuffer;
        std::shared_ptr< Translator > m_translator;
    };


} // namespace Tests
} // namespace Disruptor

using namespace Disruptor;
using namespace ::Disruptor::Tests;


TEST_F(EventPublisherFixture, ShouldPublishEvent)
{
    m_ringBuffer->addGatingSequences({ std::make_shared< NoOpEventProcessor< LongEvent > >(m_ringBuffer)->sequence() });

    m_ringBuffer->publishEvent(m_translator);
    m_ringBuffer->publishEvent(m_translator);

    EXPECT_EQ(0L + m_valueAdd, (*m_ringBuffer)[0].value);
    EXPECT_EQ(1L + m_valueAdd, (*m_ringBuffer)[1].value);
}

TEST_F(EventPublisherFixture, ShouldTryPublishEvent)
{
    m_ringBuffer->addGatingSequences({ std::make_shared< Sequence >() });

    for (auto i = 0; i < m_bufferSize; ++i)
    {
        EXPECT_EQ(m_ringBuffer->tryPublishEvent(m_translator), true);
    }

    for (auto i = 0; i < m_bufferSize; ++i)
    {
        EXPECT_EQ((*m_ringBuffer)[i].value, i + m_valueAdd);
    }

    EXPECT_EQ(m_ringBuffer->tryPublishEvent(m_translator), false);
}
