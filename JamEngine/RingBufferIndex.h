#pragma once

namespace Jam
{

// How to use:
// Item array[100] = {};
// RingBufferIndex rbi(100); // store limit is 99 items
//
// push:
//   array[rbi.GetWriteIndex()] = item;
//   rbi.Produce();
//
// pop:
//  item = array[rbi.GetReadIndex()];
//  rbi.Consume();
//
// pop all:
// while (!rbi.IsEmpty())
// {
//    item = array[rbi.GetReadIndex()];
//    rbi.Consume();
// }

// overflow policy: overwrite old data
// underflow policy: assertion

class RingBufferIndex
{
public:
    explicit RingBufferIndex(
        const SizeT _cap)
        : m_cap(_cap)
    {
    }

    [[nodiscard]] SizeT GetSize() const
    {
        return (m_tail + m_cap - m_head) % m_cap;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_head == m_tail;
    }

    [[nodiscard]] bool IsFull() const
    {
        return (m_tail + 1) % m_cap == m_head;
    }

    void Produce()
    {
        m_tail = (m_tail + 1) % m_cap;
        if (m_tail == m_head)
        {
            m_head = (m_head + 1) % m_cap;
        }
    }

    void Consume()
    {
        JAM_ASSERT(!IsEmpty(), "Ring buffer is empty, cannot consume more items");
        m_head = (m_head + 1) % m_cap;
    }

    [[nodiscard]] SizeT GetReadIndex() const
    {
        JAM_ASSERT(!IsEmpty(), "Ring buffer is empty");
        return m_head;
    }

    [[nodiscard]] SizeT GetWriteIndex() const
    {
        return m_tail;
    }

private:
    SizeT m_head = 0;
    SizeT m_tail = 0;
    SizeT m_cap  = 0;
};

}   // namespace Jam