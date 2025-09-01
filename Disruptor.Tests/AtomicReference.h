#pragma once

#include <optional>


namespace Disruptor
{
namespace Tests
{

    template <class T>
    class AtomicReference
    {
    public:
        explicit AtomicReference(const std::optional< T >& value = std::nullopt)
            : m_value(value)
        {
        }

        std::optional< T > read()
        {
            return m_value;
        }

        void write(const std::optional< T >& value)
        {
            m_value = value;
        }

    private:
        std::optional< T > m_value;
    };

} // namespace Tests
} // namespace Disruptor
