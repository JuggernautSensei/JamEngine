#pragma once
#include "String.h"

namespace Jam
{

constexpr float k_constexprStringHashMapLoadFactor    = 0.7f;
constexpr float k_invConstexprStringHashMapLoadFactor = 1.f / k_constexprStringHashMapLoadFactor;

template<SizeT k_size, typename TValue>
class ConstexprStringHashMap
{
    static_assert(k_size > 0, "ConstexprStringHashMap size must be greater than 0.");

    struct Node
    {
        StringView key       = {};
        TValue     value     = {};
        bool       bOccupied = false;
    };

    static constexpr SizeT k_invalidIdx = -1;

public:
    constexpr ConstexprStringHashMap() = default;

    constexpr void Insert(
        const StringView _key,
        const TValue&    _value)
    {
        const SizeT hash  = _key.Hash32();
        SizeT       index = hash % k_size;

        for (SizeT i = 0; i < k_size; ++i)
        {
            Node& node = m_nodes[index];
            if (!node.bOccupied)
            {
                node.key       = _key;
                node.value     = _value;
                node.bOccupied = true;
                ++m_numNodes;
                return;
            }
            if (node.key == _key)
            {
                node.value = _value;
                return;
            }

            index = (index + 1) % k_size;
        }
    }

    constexpr const TValue& Get(
        const StringView _key) const
    {
        const SizeT index = FindIndexOrInvalid_(_key);
        return m_nodes[index].value;
    }

    [[nodiscard]] constexpr bool Has(
        const StringView _key) const
    {
        return FindIndexOrInvalid_(_key) != k_invalidIdx;
    }

private:
    [[nodiscard]] constexpr SizeT FindIndexOrInvalid_(
        const StringView _key) const
    {
        if (m_numNodes == 0)
        {
            return k_invalidIdx;
        }

        const SizeT hash  = _key.Hash32();
        SizeT       index = hash % k_size;

        for (SizeT i = 0; i < k_size; ++i)
        {
            const Node& node = m_nodes[index];
            if (!node.bOccupied)
            {
                return k_invalidIdx;
            }

            if (node.key == _key)
            {
                return index;
            }

            index = (index + 1) % k_size;
        }

        return k_invalidIdx;
    }

    Array<Node, k_size> m_nodes    = {};
    SizeT               m_numNodes = 0;
};

}   // namespace Jam
