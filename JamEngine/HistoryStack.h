#pragma once

namespace jam
{

template<typename T, size_t N>
class FixedHistoryStack
{
public:
    void Push(const T& _item)
    {
        const Int32 nextIdx = IndexMod_(m_curIdx + 1);
        m_items[nextIdx]    = _item;
        m_curIdx            = nextIdx;
        m_endIdx            = IndexMod_(m_curIdx + 1);

        if (m_endIdx == m_beginIdx)
        {
            m_beginIdx = IndexMod_(m_beginIdx + 1);
        }
    }

    void Pop()
    {
        JAM_ASSERT(!IsEmpty(), "FixedHistoryStack is empty.");
        if (m_curIdx == m_beginIdx)
        {
            Clear();
        }
        else
        {
            m_curIdx = IndexMod_(m_curIdx - 1);
            m_endIdx = IndexMod_(m_curIdx + 1);
        }
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_beginIdx == m_endIdx || m_curIdx == -1;
    }

    [[nodiscard]] const T& GetCurrent() const
    {
        JAM_ASSERT(!IsEmpty(), "FixedHistoryStack is empty.");
        return m_items[m_curIdx];
    }

    void Undo()
    {
        JAM_ASSERT(CanUndo(), "Cannot undo.");
        m_curIdx = IndexMod_(m_curIdx - 1);
    }

    void Redo()
    {
        JAM_ASSERT(CanRedo(), "Cannot redo.");
        m_curIdx = IndexMod_(m_curIdx + 1);
    }

    bool CanUndo() const
    {
        return m_curIdx != m_beginIdx && !IsEmpty();
    }

    bool CanRedo() const
    {
        return m_curIdx != IndexMod_(m_endIdx - 1) && !IsEmpty();
    }

    void Clear()
    {
        m_curIdx   = -1;
        m_beginIdx = 0;
        m_endIdx   = 0;
    }

    [[nodiscard]] Int32 NumItems() const
    {
        if (m_endIdx >= m_beginIdx)
        {
            return m_endIdx - m_beginIdx;
        }
        else
        {
            return (N + 1 - m_beginIdx) + m_endIdx;
        }
    }

private:
    [[nodiscard]] Int32 IndexMod_(const Int32 v) const
    {
        return (v + (N + 1)) % (N + 1);
    }

    Array<T, N + 1> m_items    = {};
    Int32           m_curIdx   = -1;
    Int32           m_beginIdx = 0;
    Int32           m_endIdx   = 0;
};

}   // namespace jam