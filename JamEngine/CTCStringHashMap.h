#pragma once
#include "String.h"

namespace Jam
{

constexpr float kCTCStringHashMapLoadFactor    = 0.7f;
constexpr float kInvCTCStringHashMapLoadFactor = 1.f / kCTCStringHashMapLoadFactor;

template<typename TValue, SizeT kSize>
class CTCStringHashMap
{
    static_assert(kSize > 0, "CTStringHashMap size must be greater than 0.");

    struct Node
    {
        StringView key       = {};
        TValue     value     = {};
        bool       bOccupied = false;
    };

    static constexpr SizeT kInvalidIdx = -1;

public:
    constexpr CTCStringHashMap() = default;

    constexpr void Insert(
        const StringView _key,
        const TValue&    _value)
    {
        const UInt64 hash  = _key.Hash();
        SizeT        index = hash % kSize;

        for (SizeT i = 0; i < kSize; ++i)
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

            index = (index + 1) % kSize;
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
        return FindIndexOrInvalid_(_key) != kInvalidIdx;
    }

private:
    [[nodiscard]] constexpr SizeT FindIndexOrInvalid_(
        const StringView _key) const
    {
        if (m_numNodes == 0)
        {
            return kInvalidIdx;
        }

        const UInt64 hash  = _key.Hash();
        SizeT        index = hash % kSize;

        for (SizeT i = 0; i < kSize; ++i)
        {
            const Node& node = m_nodes[index];
            if (!node.bOccupied)
            {
                return kInvalidIdx;
            }

            if (node.key == _key)
            {
                return index;
            }

            index = (index + 1) % kSize;
        }

        return kInvalidIdx;
    }

    Array<Node, kSize> m_nodes    = {};
    SizeT              m_numNodes = 0;
};

}   // namespace Jam
