#pragma once

namespace Jam
{

template<
    typename TKey,
    typename TValue,
    typename Hasher   = ankerl::unordered_dense::hash<TKey>,
    typename KeyEqual = std::equal_to<TKey>>
class LruCache   // Last Recently Used Cache
{

public:
    struct Node
    {
        TKey   key;
        TValue value;
    };

    using ListT         = List<Node>;
    using Iterator      = ListT::iterator;
    using ConstIterator = ListT::const_iterator;
    using HashMapT      = HashMap<TKey, Iterator>;

    explicit LruCache(
        const SizeT capacity)
        : m_capacity(capacity)
    {
        JAM_ASSERT(m_capacity > 0, "LRUCache::LRUCache() - Capacity must be greater than zero.");
    }

    void Insert(
        const TKey&   key,
        const TValue& value)
    {
        JAM_ASSERT(m_capacity > 0, "LRUCache::InsertEncode() - Capacity must be greater than zero.");

        // update existing key
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            it->second->value = value;
            UpdateUsage_(it->second);
        }
        else
        {
            m_items.push_front(Node { key, value });
            m_map.emplace(key, m_items.begin());

            RemoveOldestUntilFit_();   // evict oldest if over capacity
        }
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    void Insert(
        UKey&&        key,
        const TValue& value)
    {
        JAM_ASSERT(m_capacity > 0, "LRUCache::InsertEncode() - Capacity must be greater than zero.");

        // update existing key
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            it->second->value = value;
            UpdateUsage_(it->second);
        }
        else
        {
            m_items.push_front(Node { std::forward<UKey>(key), value });
            m_map.emplace(std::forward<UKey>(key), m_items.begin());

            RemoveOldestUntilFit_();   // evict oldest if over capacity
        }
    }

    void Remove(
        const TKey& key)
    {
        auto mapIter = m_map.find(key);
        if (mapIter != m_map.end())
        {
            m_items.erase(mapIter->second);
            m_map.erase(mapIter);
        }
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    void Remove(
        UKey&& key)
    {
        auto mapIter = m_map.find(key);
        if (mapIter != m_map.end())
        {
            m_items.erase(mapIter->second);
            m_map.erase(mapIter);
        }
    }

    void Clear()
    {
        m_items.clear();
        m_map.clear();
    }

    void RemoveOldest()
    {
        JAM_ASSERT(GetSize() > 0, "LRUCache::RemoveOldest() - No items to remove.");

        const Node& oldest = m_items.back();
        m_map.erase(oldest.key);
        m_items.pop_back();
    }

    [[nodiscard]] const TValue& At(
        const TKey& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second->value;
        }
        else
        {
            JAM_UNREACHABLE;
        }
    }

    TValue& At(
        const TKey& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second->value;
        }
        else
        {
            Insert(key, TValue {});
            return At(key);
        }
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] const TValue& At(
        UKey&& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second->value;
        }
        else
        {
            JAM_UNREACHABLE;
        }
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] TValue& At(
        UKey&& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second->value;
        }

        Insert(std::forward<UKey>(key), TValue {});
        return At(std::forward<UKey>(key));
    }

    [[nodiscard]] const TValue& operator[](
        const TKey& key) const
    {
        return At(key);
    }

    TValue& operator[](
        const TKey& key)
    {
        return At(key);
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] const TValue& operator[](
        UKey&& key) const
    {
        return At(std::forward<UKey>(key));
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] TValue& operator[](
        UKey&& key)
    {
        return At(std::forward<UKey>(key));
    }

    [[nodiscard]] bool Contains(
        const TKey& key) const
    {
        return m_map.contains(key);
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] bool Contains(
        UKey&& key) const
    {
        return m_map.contains(std::forward<UKey>(key));
    }

    [[nodiscard]] SizeT GetSize() const
    {
        return m_items.size();
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_items.empty();
    }

    void SetCapacity(
        const SizeT capacity)
    {
        JAM_ASSERT(capacity > 0, "LRUCache::SetCapacity() - Capacity must be greater than zero.");
        m_capacity = capacity;

        RemoveOldestUntilFit_();   // evict oldest if over new capacity
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return m_capacity;
    }

    [[nodiscard]] Iterator Find(
        const TKey& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second;
        }

        return m_items.end();
    }

    [[nodiscard]] ConstIterator Find(
        const TKey& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            return it->second;
        }

        return m_items.end();
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] Iterator Find(
        UKey&& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second;
        }

        return m_items.end();
    }

    template<typename UKey>
        requires ankerl::unordered_dense::detail::is_transparent_v<Hasher, KeyEqual>
    [[nodiscard]] ConstIterator Find(
        UKey&& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            return it->second;
        }

        return m_items.end();
    }

    [[nodiscard]] Iterator Begin()
    {
        return m_items.begin();
    }

    [[nodiscard]] Iterator End()
    {
        return m_items.end();
    }

    [[nodiscard]] ConstIterator Begin() const
    {
        return m_items.begin();
    }

    [[nodiscard]] ConstIterator End() const
    {
        return m_items.end();
    }

private:
    void RemoveOldestUntilFit_()
    {
        while (GetSize() > m_capacity)
        {
            RemoveOldest();
        }
    }

    void UpdateUsage_(const Iterator _iter)
    {
        m_items.splice(m_items.begin(), m_items, _iter);
    }

    ListT    m_items    = {};   // front: newest, back: oldest
    HashMapT m_map      = {};
    SizeT    m_capacity = 0;
};

template<typename TValue, typename KeyEqual = std::equal_to<TValue>>
using StringLRUCache = LruCache<String, TValue, Details::StringHashMapHasher, KeyEqual>;

}   // namespace Jam
