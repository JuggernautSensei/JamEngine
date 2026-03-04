#pragma once

namespace Jam
{

template<typename TKey, typename TValue>
class LRUCache   // Last Recently Used Cache
{
    struct Node
    {
        TKey   key;
        TValue value;
    };

public:
    using Iter      = List<Node>::iterator;
    using ConstIter = List<Node>::const_iterator;

    explicit LRUCache(const SizeT capacity)
        : m_capacity(capacity)
    {
        JAM_ASSERT(m_capacity > 0, "LRUCache::LRUCache() - Capacity must be greater than zero.");
    }

    void Insert(const TKey& key, const TValue& value)
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

            // evict oldest if over capacity
            RemoveOldestUntilFit_();
        }
    }

    void Insert(const TKey& key, TValue&& value)
    {
        JAM_ASSERT(m_capacity > 0, "LRUCache::InsertEncode() - Capacity must be greater than zero.");

        // update existing key
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            it->second->value = std::move(value);
            UpdateUsage_(it->second);
        }
        else
        {
            m_items.push_front(Node { key, std::move(value) });
            m_map.emplace(key, m_items.begin());

            // evict oldest if over capacity
            RemoveOldestUntilFit_();
        }
    }

    void Remove(const TKey& key)
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
        JAM_ASSERT(GetNumItems() > 0, "LRUCache::RemoveOldest() - No items to remove.");

        const Node& oldest = m_items.back();
        m_map.erase(oldest.key);
        m_items.pop_back();
    }

    [[nodiscard]] const TValue& At(const TKey& key) const
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

    TValue& At(const TKey& key)
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

    [[nodiscard]] const TValue& operator[](const TKey& key) const
    {
        return At(key);
    }

    TValue& operator[](const TKey& key)
    {
        return At(key);
    }

    [[nodiscard]] bool Contains(const TKey& key) const
    {
        return m_map.contains(key);
    }

    [[nodiscard]] SizeT GetNumItems() const
    {
        return m_items.size();
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_items.empty();
    }

    void SetCapacity(const SizeT capacity)
    {
        JAM_ASSERT(capacity > 0, "LRUCache::SetCapacity() - Capacity must be greater than zero.");
        m_capacity = capacity;

        // evict oldest if over new capacity
        RemoveOldestUntilFit_();
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return m_capacity;
    }

    [[nodiscard]] Iter Find(const TKey& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            UpdateUsage_(it->second);
            return it->second;
        }
        else
        {
            return m_items.end();
        }
    }

    [[nodiscard]] ConstIter Find(const TKey& key) const
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            return it->second;
        }
        else
        {
            return m_items.end();
        }
    }

    Iter Begin()
    {
        return m_items.begin();
    }

    Iter End()
    {
        return m_items.end();
    }

    ConstIter Begin() const
    {
        return m_items.begin();
    }

    ConstIter End() const
    {
        return m_items.end();
    }

private:
    void RemoveOldestUntilFit_()
    {
        while (GetNumItems() > m_capacity)
        {
            RemoveOldest();
        }
    }

    void UpdateUsage_(const Iter _iter)
    {
        m_items.splice(m_items.begin(), m_items, _iter);
    }

    List<Node>          m_items    = {};   // front: newest, back: oldest
    HashMap<TKey, Iter> m_map      = {};
    SizeT               m_capacity = 0;
};

}   // namespace Jam
