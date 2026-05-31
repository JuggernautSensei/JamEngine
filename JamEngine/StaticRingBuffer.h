#pragma once
#include "RingBufferIndex.h"

namespace Jam
{

template<typename TValue, SizeT kCap>
class StaticRingBuffer
{
public:
    void Push(const TValue& _value)
    {
        m_buffer[m_controller.GetWriteIndex()] = _value;
        m_controller.Reserve();
    }

    void Push(TValue&& _value)
    {
        m_buffer[m_controller.GetWriteIndex()] = std::move(_value);
        m_controller.Reserve();
    }

    TValue& Front()
    {
        return m_buffer[m_controller.GetReadIndex()];
    }

    const TValue& Front() const
    {
        return m_buffer[m_controller.GetReadIndex()];
    }

    void Pop()
    {
        ~m_buffer[m_controller.GetReadIndex()];
        m_controller.Consume();
    }

    [[nodiscard]] Int32 GetSize() const
    {
        return m_controller.GetSize();
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_controller.IsEmpty();
    }

    [[nodiscard]] bool IsFull() const
    {
        return m_controller.IsFull();
    }

private:
    Array<TValue, kCap>  m_buffer = {};
    RingBufferIndex m_controller { kCap };
};

}   // namespace Jam