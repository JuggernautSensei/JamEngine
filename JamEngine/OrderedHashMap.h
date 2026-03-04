#pragma once

namespace Jam
{

// OrederdHashMap / Set
// 특징
// 1. 삽입 순서를 보장함.
// 2. 키의 유일성을 보장함.
// 3. 삽입 / 삭제 / 탐색 모두 O(1) (평균적으로)

template<
    typename TKey,
    typename TValue,
    typename THasher   = ankerl::unordered_dense::hash<TKey>,
    typename TKeyEqual = std::equal_to<TKey>>
class OrderedHashMap
{
public:
    struct Node
    {
        Node(const TKey& _key, const TValue& _value)
            : key(_key)
            , value(_value)
        {
        }

        const TKey key;   // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        TValue     value;
    };

    using ListT             = List<Node>;
    using ListIterator      = typename ListT::iterator;
    using ListConstIterator = typename ListT::const_iterator;
    using Iterator          = typename ListT::iterator;
    using ConstIterator     = typename ListT::const_iterator;
    using HashMapT          = HashMap<TKey, Iterator, THasher, TKeyEqual>;

    struct InsertResult
    {
        Iterator iter       = {};   // 삽입된 요소를 가리킴. 삽입이 실패한 경우 End()를 가리킴.
        bool     bSucceeded = false;
    };

    struct RemoveResult
    {
        Iterator iter       = {};   // 유효한 경우 삭제된 요소의 다음 요소를 가리킴. 삭제된 요소가 마지막 요소인 경우 End()를 가리킴.
        bool     bSucceeded = false;
    };

    InsertResult PushBack(
        const TKey&   _key,
        const TValue& _value)
    {
        return Insert(End(), _key, _value);
    }

    InsertResult PushFront(
        const TKey&   _key,
        const TValue& _value)
    {
        return Insert(Begin(), _key, _value);
    }

    InsertResult Insert(
        const ConstIterator _position,
        const TKey&         _key,
        const TValue&       _value)
    {
        auto [mapIter, bInserted] = m_keyToIter.emplace(_key, m_values.end());
        if (bInserted)
        {
            mapIter->second = m_values.emplace(_position, _key, _value);
        }
        return { mapIter->second, bInserted };
    }

    RemoveResult Remove(
        const TKey& key)
    {
        auto mapIter = m_keyToIter.find(key);
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
        auto mapIter = m_keyToIter.find(_position->key);
        if (mapIter == m_keyToIter.end())
        {
            return {};
        }

        const ListIterator it = m_values.erase(_position);
        m_keyToIter.erase(mapIter);
        return { it, true };
    }

    TValue& Get(
        const TKey& key)
    {
        auto mapIter = m_keyToIter.find(key);
        JAM_ASSERT(mapIter != m_keyToIter.end());
        return mapIter->second->value;
    }

    const TValue& Get(
        const TKey& key) const
    {
        auto mapIter = m_keyToIter.find(key);
        JAM_ASSERT(mapIter != m_keyToIter.end());
        return mapIter->second->value;
    }

    void Splice(
        const TKey& _from,
        const TKey& _to)
    {
        auto fromIter = m_keyToIter.find(_from);
        if (fromIter == m_keyToIter.end())
        {
            return;
        }

        auto toIter = m_keyToIter.find(_to);
        if (toIter == m_keyToIter.end())
        {
            return;
        }

        Splice(fromIter->second, toIter->second);
    }

    void Splice(
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
        const TKey& key) const
    {
        return m_keyToIter.find(key) != m_keyToIter.end();
    }

    [[nodiscard]] Iterator Find(
        const TKey& key)
    {
        auto mapIter = m_keyToIter.find(key);
        if (mapIter == m_keyToIter.end())
        {
            return m_values.end();
        }
        return mapIter->second;
    }

    [[nodiscard]] ConstIterator Find(
        const TKey& key) const
    {
        auto mapIter = m_keyToIter.find(key);
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

    [[nodiscard]] SizeT GetSize() const
    {
        return m_values.size();
    }

    void Clear()
    {
        m_values.clear();
        m_keyToIter.clear();
    }

    template<typename TCompare>
    void SortByKey(TCompare compare = eastl::greater<Node> {})
    {
        m_values.sort(compare);
        for (auto iter = m_values.begin(); iter != m_values.end(); ++iter)
        {
            auto mapIter = m_keyToIter.find(iter->key);
            JAM_ASSERT(mapIter != m_keyToIter.end());
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

template<typename TValue>
using OrderedStringHashMap = OrderedHashMap<String, TValue, Details::StringHashMapHasher>;

}   // namespace Jam
