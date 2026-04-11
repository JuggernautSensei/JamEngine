#pragma once

namespace Jam
{

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
        TKey   key;
        TValue value;
    };

    using ListT             = List<Node>;
    using ListIterator      = typename ListT::iterator;
    using ListConstIterator = typename ListT::const_iterator;

    template<bool k_bIsConst>
    class IteratorBase
    {
    public:
        using ListIterT = std::conditional_t<k_bIsConst, ListConstIterator, ListIterator>;

        struct Proxy
        {
            using ValueT = std::conditional_t<k_bIsConst, const TValue, TValue>;

            const Proxy* operator->() const
            {
                return this;
            }

            Proxy* operator->()
            {
                return this;
            }

            const TKey& key;
            ValueT&     value;
        };

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Node;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<k_bIsConst, const Node*, Node*>;
        using reference         = std::conditional_t<k_bIsConst, const Node&, Node&>;

        IteratorBase()                               = default;
        ~IteratorBase()                              = default;
        IteratorBase(const IteratorBase&)            = default;
        IteratorBase& operator=(const IteratorBase&) = default;
        IteratorBase(IteratorBase&&)                 = default;
        IteratorBase& operator=(IteratorBase&&)      = default;

        /* implicit */ IteratorBase(ListIterT listIter)
            : m_listIter(listIter)
        {
        }

        template<bool k_bOtherIsConst>
            requires(k_bIsConst || !k_bOtherIsConst)
        /* implicit */ IteratorBase(const IteratorBase<k_bOtherIsConst>& other)
            : m_listIter(other.m_listIter)
        {
        }

        Proxy operator*() const
        {
            return { m_listIter->key, m_listIter->value };
        }

        Proxy operator->() const
        {
            return { m_listIter->key, m_listIter->value };
        }

        Proxy operator*()
        {
            return { m_listIter->key, m_listIter->value };
        }

        Proxy operator->()
        {
            return { m_listIter->key, m_listIter->value };
        }

        IteratorBase& operator++()
        {
            ++m_listIter;
            return *this;
        }

        IteratorBase operator++(int)
        {
            IteratorBase tmp = *this;
            ++(*this);
            return tmp;
        }

        IteratorBase& operator--()
        {
            --m_listIter;
            return *this;
        }

        IteratorBase operator--(int)
        {
            IteratorBase tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const IteratorBase& other) const
        {
            return m_listIter == other.m_listIter;
        }

        bool operator!=(const IteratorBase& other) const
        {
            return m_listIter != other.m_listIter;
        }

    private:
        ListIterT m_listIter = {};
        friend class OrderedHashMap;
        friend class IteratorBase<!k_bIsConst>;
    };

    using Iterator      = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;
    using HashMapT      = HashMap<TKey, ListIterator, THasher, TKeyEqual>;

    struct InsertResult
    {
        Iterator iter;
        bool     bSucceeded = false;
    };

    struct RemoveResult
    {
        Iterator iter;
        bool     bSucceeded = false;
    };

    OrderedHashMap()  = default;
    ~OrderedHashMap() = default;

    OrderedHashMap(const OrderedHashMap& other)
    {
        *this = other;
    }

    OrderedHashMap& operator=(const OrderedHashMap& other)
    {
        if (this != &other)
        {
            Clear();
            for (const auto& node: other.m_values)
            {
                PushBack(node.key, node.value);
            }
        }
        return *this;
    }

    OrderedHashMap(OrderedHashMap&& other) noexcept
        : m_values(std::move(other.m_values))
        , m_keyToIter(std::move(other.m_keyToIter))
    {
    }

    OrderedHashMap& operator=(OrderedHashMap&& other) noexcept
    {
        if (this != &other)
        {
            m_values    = std::move(other.m_values);
            m_keyToIter = std::move(other.m_keyToIter);
        }
        return *this;
    }

    // push back / front

    InsertResult PushBack(const TKey& key, const TValue& value)
    {
        return Insert(End(), key, value);
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, UKey>
    InsertResult PushBack(const UKey& key, const TValue& value)
    {
        return Insert(End(), key, value);
    }

    InsertResult PushFront(const TKey& key, const TValue& value)
    {
        return Insert(Begin(), key, value);
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, UKey>
    InsertResult PushFront(const UKey& key, const TValue& value)
    {
        return Insert(Begin(), key, value);
    }

    // insert

    InsertResult Insert(ConstIterator position, const TKey& key, const TValue& value)
    {
        auto [mapIter, bInserted] = m_keyToIter.emplace(key, m_values.end());
        if (bInserted)
        {
            mapIter->second = m_values.emplace(position.m_listIter, key, value);
            return { Iterator(mapIter->second), true };
        }
        return { End(), false };
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, UKey>
    InsertResult Insert(ConstIterator position, const UKey& key, const TValue& value)
    {
        auto [mapIter, bInserted] = m_keyToIter.emplace(key, m_values.end());
        if (bInserted)
        {
            mapIter->second = m_values.emplace(position.m_listIter, key, value);
            return { Iterator(mapIter->second), true };
        }
        return { End(), false };
    }

    // find

    [[nodiscard]] Iterator Find(const TKey& key)
    {
        auto it = m_keyToIter.find(key);
        return (it != m_keyToIter.end()) ? Iterator(it->second) : End();
    }

    [[nodiscard]] ConstIterator Find(const TKey& key) const
    {
        auto it = m_keyToIter.find(key);
        return (it != m_keyToIter.end()) ? ConstIterator(it->second) : End();
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    [[nodiscard]] Iterator Find(const K& key)
    {
        auto it = m_keyToIter.find(key);
        return (it != m_keyToIter.end()) ? Iterator(it->second) : End();
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    [[nodiscard]] ConstIterator Find(const K& key) const
    {
        auto it = m_keyToIter.find(key);
        return (it != m_keyToIter.end()) ? ConstIterator(it->second) : End();
    }

    // contains

    [[nodiscard]] bool Contains(const TKey& key) const
    {
        return m_keyToIter.contains(key);
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    [[nodiscard]] bool Contains(const K& key) const
    {
        return m_keyToIter.contains(key);
    }

    // get

    TValue& Get(const TKey& key)
    {
        auto it = m_keyToIter.find(key);
        JAM_ASSERT(it != m_keyToIter.end(), "Key not found in map");
        return it->second->value;
    }

    const TValue& Get(const TKey& key) const
    {
        auto it = m_keyToIter.find(key);
        JAM_ASSERT(it != m_keyToIter.end(), "Key not found in map");
        return it->second->value;
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    TValue& Get(const K& key)
    {
        auto it = m_keyToIter.find(key);
        JAM_ASSERT(it != m_keyToIter.end(), "Key not found in map");
        return it->second->value;
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    const TValue& Get(const K& key) const
    {
        auto it = m_keyToIter.find(key);
        JAM_ASSERT(it != m_keyToIter.end(), "Key not found in map");
        return it->second->value;
    }

    // remove

    RemoveResult Remove(const TKey& key)
    {
        auto mapIter = m_keyToIter.find(key);
        if (mapIter == m_keyToIter.end())
        {
            return { End(), false };
        }

        auto nextListIter = m_values.erase(mapIter->second);
        m_keyToIter.erase(mapIter);
        return { Iterator(nextListIter), true };
    }

    template<typename K>
        requires ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K>
    RemoveResult Remove(const K& key)
    {
        auto mapIter = m_keyToIter.find(key);
        if (mapIter == m_keyToIter.end())
        {
            return { End(), false };
        }

        auto nextListIter = m_values.erase(mapIter->second);
        m_keyToIter.erase(mapIter);
        return { Iterator(nextListIter), true };
    }

    RemoveResult Remove(ConstIterator position)
    {
        if (position == End())
        {
            return { End(), false };
        }

        auto mapIter = m_keyToIter.find(position->key);
        JAM_ASSERT(mapIter != m_keyToIter.end(), "Position iterator's key not found in map");

        auto nextListIter = m_values.erase(position.m_listIter);
        m_keyToIter.erase(mapIter);
        return { Iterator(nextListIter), true };
    }

    // []

    TValue& operator[](const TKey& key)
    {
        auto mapIter = m_keyToIter.find(key);
        if (mapIter == m_keyToIter.end())
        {
            auto res = PushBack(key, TValue {});
            return res.iter->value;
        }
        return mapIter->second->value;
    }

    // front / back

    Node& Front()
    {
        JAM_ASSERT(!IsEmpty(), "Cannot call Front() on an empty map.");
        return m_values.front();
    }

    const Node& Front() const
    {
        JAM_ASSERT(!IsEmpty(), "Cannot call Front() on an empty map.");
        return m_values.front();
    }

    Node& Back()
    {
        JAM_ASSERT(!IsEmpty(), "Cannot call Back() on an empty map.");
        return m_values.back();
    }

    const Node& Back() const
    {
        JAM_ASSERT(!IsEmpty(), "Cannot call Front() on an empty map.");
        return m_values.back();
    }

    // misc

    void Move(const TKey& _from, const TKey& _to)
    {
        auto fromIt = m_keyToIter.find(_from);
        auto toIt   = m_keyToIter.find(_to);
        if (fromIt == m_keyToIter.end() || toIt == m_keyToIter.end())
        {
            return;
        }
        Move(ConstIterator { fromIt->second }, ConstIterator { toIt->second });
    }

    template<typename K1, typename K2>
        requires(ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K1>
                 && ankerl::unordered_dense::detail::is_transparent_v<TKeyEqual, K2>)
    void Move(const K1& _from, const K2& _to)
    {
        auto fromIt = m_keyToIter.find(_from);
        auto toIt   = m_keyToIter.find(_to);
        if (fromIt == m_keyToIter.end() || toIt == m_keyToIter.end())
        {
            return;
        }

        Move(ConstIterator { fromIt->second }, ConstIterator { toIt->second });
    }

    void Move(ConstIterator _fromIt, ConstIterator _toIt)
    {
        if (_toIt == _fromIt)
        {
            return;
        }
        m_values.splice(_toIt.m_listIter, m_values, _fromIt.m_listIter);
    }

    void Clear()
    {
        m_values.clear();
        m_keyToIter.clear();
    }

    [[nodiscard]] SizeT GetSize() const
    {
        return m_values.size();
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_values.empty();
    }

    Iterator Begin()
    {
        return Iterator(m_values.begin());
    }

    Iterator End()
    {
        return Iterator(m_values.end());
    }

    ConstIterator Begin() const
    {
        return ConstIterator(m_values.begin());
    }

    ConstIterator End() const
    {
        return ConstIterator(m_values.end());
    }

    // stl compat

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

    SizeT size() const
    {
        return m_values.size();
    }

    bool empty() const
    {
        return m_values.empty();
    }

private:
    ListT    m_values    = {};
    HashMapT m_keyToIter = {};
};

template<typename TValue>
using OrderedStringHashMap = OrderedHashMap<String, TValue, Details::StringHashMapHasher>;

}   // namespace Jam