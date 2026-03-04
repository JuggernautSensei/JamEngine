#pragma once

namespace Jam
{

template<typename T, SizeT k_cap>
class StackList
{
    struct Node
    {
        T     value  = {};
        bool  bValid = false;
        Node* pNext  = nullptr;
        Node* pPrev  = nullptr;
    };

public:
    class Iterator
    {
    public:
        using ValueT = T;
        using NodeT  = Node;

        Iterator() = default;

        explicit Iterator(NodeT* _pNode)
            : m_pNode(_pNode)
        {
        }

        ValueT& operator*() const
        {
            return m_pNode->value;
        }

        ValueT* operator->() const
        {
            return &(**this);
        }

        Iterator& operator++()
        {
            if (m_pNode)
            {
                m_pNode = m_pNode->pNext;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        Iterator& operator--()
        {
            if (m_pNode)
            {
                m_pNode = m_pNode->pPrev;
            }
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const
        {
            return m_pNode == other.m_pNode;
        }

        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

        NodeT* GetNode() const
        {
            return m_pNode;
        }

    private:
        NodeT* m_pNode = nullptr;
    };

    using ConstIterator = const Iterator;

    void PushBack(
        const T& _value)
    {
        Insert(End(), _value);
    }

    void PopBack()
    {
        JAM_ASSERT(!IsEmpty(), "cannot pop from an empty list");
        Remove(Iterator { m_pTail });
    }

    void Insert(
        ConstIterator _it,
        const T&      _value)
    {
        Node* pNode   = AllocateNode_();
        pNode->value  = _value;
        pNode->bValid = true;

        if (_it == End())
        {
            pNode->pNext = nullptr;
            pNode->pPrev = nullptr;

            if (m_pTail)
            {
                m_pTail->pNext = pNode;
                pNode->pPrev   = m_pTail;
            }

            m_pTail = pNode;

            if (m_pHead == nullptr)
            {
                m_pHead = pNode;
            }
        }
        else
        {
            Node* pCurrent = _it.GetNode();
            Node* pPrev    = pCurrent->pPrev;

            pNode->pNext    = pCurrent;
            pNode->pPrev    = pPrev;
            pCurrent->pPrev = pNode;

            if (pPrev)
            {
                pPrev->pNext = pNode;
            }
            else
            {
                m_pHead = pNode;
            }
        }
        ++m_size;
    }

    void Remove(
        ConstIterator _it)
    {
        JAM_ASSERT(_it != End(), "invalid iterator");

        Node* pNode = _it.GetNode();

        if (pNode->pPrev)
        {
            pNode->pPrev->pNext = pNode->pNext;
        }
        else
        {
            m_pHead = pNode->pNext;
        }

        if (pNode->pNext)
        {
            pNode->pNext->pPrev = pNode->pPrev;
        }
        else
        {
            m_pTail = pNode->pPrev;
        }

        pNode->bValid = false;
        pNode->pNext  = m_freeList;
        m_freeList    = pNode;
        --m_size;
    }

    void Swap(
        ConstIterator _from,
        ConstIterator _to)
    {
        JAM_ASSERT(_from != End() && _to != End(), "invalid iterator");
        if (_from == _to)
        {
            return;
        }

        Node* pFrom = _from.GetNode();
        Node* pTo   = _to.GetNode();
        std::swap(pFrom->value, pTo->value);
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_pHead == nullptr;
    }

    Iterator Begin()
    {
        return Iterator(m_pHead);
    }

    Iterator End()
    {
        return Iterator(nullptr);
    }

    ConstIterator Begin() const
    {
        return ConstIterator(m_pHead);
    }

    ConstIterator End() const
    {
        return ConstIterator(nullptr);
    }

    ////////// std /////////

    Iterator begin()
    {
        return Begin();
    }

    Iterator end()
    {
        return End();
    }

    ConstIterator begin() const
    {
        return Begin();
    }

    ConstIterator end() const
    {
        return End();
    }

private:
    Node* AllocateNode_()
    {
        if (m_freeList)
        {
            Node* pNode = m_freeList;
            m_freeList  = m_freeList->pNext;
            return pNode;
        }

        return &m_nodes[m_stackTop++];
    }

    Array<Node, k_cap> m_nodes    = {};
    SizeT              m_size     = 0;
    SizeT              m_stackTop = 0;
    Node*              m_freeList = nullptr;
    Node*              m_pHead    = nullptr;
    Node*              m_pTail    = nullptr;
};

}   // namespace Jam