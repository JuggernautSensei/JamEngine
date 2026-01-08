#pragma once

namespace jam
{

template<typename T>
class HistoryStack
{
public:
    void Push(const T& _item)
    {
        m_items.push_back(_item);
        m_redoItems.clear();
    }

    const T& Undo()
    {
        JAM_ASSERT(CanUndo(), "HistoryStack::Undo() - Cannot undo.");
        m_redoItems.push_back(m_items.back());
        m_items.pop_back();
        return m_redoItems.back();   // return the undone item
    }

    const T& Redo()
    {
        JAM_ASSERT(CanRedo(), "HistoryStack::Redo() - Cannot redo.");
        m_items.push_back(m_redoItems.back());
        m_redoItems.pop_back();
        return m_items.back();   // return the redone item
    }

    bool CanUndo() const
    {
        return !m_items.empty();
    }

    bool CanRedo() const
    {
        return !m_redoItems.empty();
    }

    void Clear()
    {
        m_items.clear();
        m_redoItems.clear();
    }

private:
    List<T> m_items;
    List<T> m_redoItems;
};

}   // namespace jam