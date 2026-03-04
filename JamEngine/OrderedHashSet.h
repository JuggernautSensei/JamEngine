#pragma once

namespace Jam
{

// 삽입 순서를 보장하는 셋
// 삽입 / 삭제 / 탐색 모두 O(1)

template<
    typename TKey,
    typename THasher   = ankerl::unordered_dense::hash<TKey>,
    typename TKeyEqual = std::equal_to<TKey>>
class OrderedHashSet
{
public:
    using ListT             = List<const TKey>;
    using ListIterator      = typename ListT::iterator;
    using ListConstIterator = typename ListT::const_iterator;
    using Iterator          = typename ListT::iterator;
    using ConstIterator     = typename ListT::const_iterator;
    using HashMapT          = HashMap<TKey, Iterator, THasher, TKeyEqual>;

    struct InsertResult
    {
        Iterator iter       = {};
        bool     bSucceeded = false;
    };

    struct RemoveResult
    {
        Iterator iter       = {};
        bool     bSucceeded = false;
    };

    InsertResult PushBack(
        const TKey& _key)
    {
        return Insert(End(), _key);
    }

    InsertResult PushFront(
        const TKey& _key)
    {
        return Insert(Begin(), _key);
    }

    InsertResult Insert(
        const ConstIterator _position,
        const TKey&         _key)
    {
        auto [mapIter, bInserted] = m_keyToIter.emplace(_key, m_values.end());
        if (bInserted)
        {
            mapIter->second = m_values.emplace(_position, _key);
        }
        return { mapIter->second, bInserted };
    }

    RemoveResult Remove(
        const TKey& _key)
    {
        auto mapIter = m_keyToIter.find(_key);
        if (mapIter == m_keyToIter.end())
        {
            return {};
        }

        const ListIterator it = m_values.erase(mapIter->second);
        m_keyToIter.erase(mapIter);
        return { it, true };
    }

    RemoveResult Remove(
        const ConstIterator _position)
    {
        auto mapIter = m_keyToIter.find(*_position);
        if (mapIter == m_keyToIter.end())
        {
            return {};
        }

        const ListIterator it = m_values.erase(_position);
        m_keyToIter.erase(mapIter);
        return { it, true };
    }

    void Swap(
        const ConstIterator _from,
        const ConstIterator _to)
    {
        if (_from == _to)
        {
            return;
        }

        m_values.splice(_to, m_values, _from);
    }

    [[nodiscard]] bool Contains(
        const TKey& _key) const
    {
        return m_keyToIter.find(_key) != m_keyToIter.end();
    }

    [[nodiscard]] Iterator Find(
        const TKey& _key)
    {
        auto mapIter = m_keyToIter.find(_key);
        if (mapIter == m_keyToIter.end())
        {
            return m_values.end();
        }
        return mapIter->second;
    }

    [[nodiscard]] ConstIterator Find(
        const TKey& _key) const
    {
        auto mapIter = m_keyToIter.find(_key);
        if (mapIter == m_keyToIter.end())
        {
            return m_values.end();
        }
        return mapIter->second;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_values.empty();
    }

    [[nodiscard]] SizeT Size() const
    {
        return m_values.size();
    }

    void Clear()
    {
        m_values.clear();
        m_keyToIter.clear();
    }

    template<typename TCompare>
    void Sort(
        TCompare _compare)
    {
        m_values.sort(_compare);
        for (auto iter = m_values.begin(); iter != m_values.end(); ++iter)
        {
            auto mapIter = m_keyToIter.find(*iter);
            ASSERT(mapIter != m_keyToIter.end());
            mapIter->second = iter;
        }
    }

    [[nodiscard]] Iterator Begin()
    {
        return m_values.begin();
    }

    [[nodiscard]] Iterator End()
    {
        return m_values.end();
    }

    [[nodiscard]] ConstIterator Begin() const
    {
        return m_values.begin();
    }

    [[nodiscard]] ConstIterator End() const
    {
        return m_values.end();
    }

    ///////// std //////////

    [[nodiscard]] Iterator begin()
    {
        return m_values.begin();
    }

    [[nodiscard]] Iterator end()
    {
        return m_values.end();
    }

    [[nodiscard]] ConstIterator begin() const
    {
        return m_values.begin();
    }

    [[nodiscard]] ConstIterator end() const
    {
        return m_values.end();
    }

private:
    ListT    m_values    = {};
    HashMapT m_keyToIter = {};
};

using OrderedStringHashSet = OrderedHashSet<String, Details::StringHashMapHasher>;

}   // namespace Jam