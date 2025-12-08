#pragma once

namespace jam
{

template<typename T>
class HistoryStack
{
public:
    void Push(const T& _item)
    {
        if (m_cur == m_end)   // end of stack
        {
            m_stack.push_back(_item);
        }
        else if (m_cur < m_end)   // middle of stack
        {
            m_stack[m_cur] = _item;
        }
        else   // never reach
        {
            JAM_ASSERT(0, "HistoryStack top index out of bounds");
        }

        ++m_cur;
        m_end = m_cur;
    }

    [[nodiscard]] bool CanUndo() const
    {
        return m_cur > 1;
    }

    [[nodiscard]] bool CanRedo() const
    {
        return m_cur < m_end;
    }

    void Undo()
    {
        JAM_ASSERT(CanUndo(), "Cannot undo in HistoryStack");
        --m_cur;
    }

    void Redo()
    {
        JAM_ASSERT(CanRedo(), "Cannot redo in HistoryStack");
        ++m_cur;
    }

    [[nodiscard]] const T& Current() const
    {
        JAM_ASSERT(m_cur > 0 && m_cur <= m_end, "No current item in HistoryStack");
        return m_stack[m_cur - 1];
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_end == 0;
    }

    void Clear()
    {
        m_stack.clear();
        m_cur = 0;
        m_end = 0;
    }

private:
    std::vector<T> m_stack;
    Int32          m_cur = 0;
    Int32          m_end = 0;
};

}   // namespace jam