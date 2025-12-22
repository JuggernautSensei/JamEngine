#pragma once

namespace jam
{

namespace radix_detail
{
    constexpr UInt32 k_bits        = 11;
    constexpr UInt32 k_bucketCount = 1u << k_bits;
    constexpr UInt32 k_mask        = k_bucketCount - 1u;
}   // namespace radix_detail

// 32 bit key
void RadixSort(UInt32* keys, UInt32* tempKeys, UInt32 n);

// 32 bit key + values
template<class T>
void RadixSort(UInt32* keys, UInt32* tempKeys, T* values, T* tempValues, const UInt32 n)
{
    static_assert(std::is_trivially_move_assignable_v<T>, "Value type must be trivially move assignable.");
    if (n <= 1) return;

    UInt32* inK  = keys;
    UInt32* outK = tempKeys;
    T*      inV  = values;
    T*      outV = tempValues;

    UInt32 hist[radix_detail::k_bucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 3; ++pass, shift = static_cast<UInt16>(shift + radix_detail::k_bits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            UInt32 prev = inK[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const UInt32 k = inK[i];
                ++hist[(k >> shift) & radix_detail::k_mask];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted)
            break;

        // prefix-sum
        UInt32 sum = 0;
        for (UInt32 b = 0; b < radix_detail::k_bucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]  = sum;
            sum += c;
        }

        // stable scatter (key+value)
        for (UInt32 i = 0; i < n; ++i)
        {
            const UInt32 k   = inK[i];
            const UInt32 b   = (k >> shift) & radix_detail::k_mask;
            UInt32 dst = hist[b]++;
            outK[dst]  = k;
            outV[dst]  = inV[i];
        }

        // swap buffers
        {
            const auto tmp = inK;
            inK      = outK;
            outK     = tmp;
        }
        {
            auto tmp = inV;
            inV      = outV;
            outV     = tmp;
        }
    }

    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(UInt32) * n);
        for (UInt32 i = 0; i < n; ++i)
            values[i] = tempValues[i];
    }
}

// 64-bit key only
void RadixSort(uint64_t* keys, uint64_t* tempKeys, UInt32 n);

// 64-bit key + values
template<class T>
void RadixSort(uint64_t* keys, uint64_t* tempKeys, T* values, T* tempValues, const UInt32 n)
{
    static_assert(std::is_trivially_move_assignable_v<T>, "Value type must be trivially move assignable.");
    if (n <= 1) return;

    uint64_t* inK  = keys;
    uint64_t* outK = tempKeys;
    T*        inV  = values;
    T*        outV = tempValues;

    UInt32 hist[radix_detail::k_bucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 6; ++pass, shift = static_cast<UInt16>(shift + radix_detail::k_bits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            uint64_t prev = inK[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const uint64_t k = inK[i];
                ++hist[static_cast<UInt32>((k >> shift) & radix_detail::k_mask)];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted)
            break;

        UInt32 sum = 0;
        for (UInt32 b = 0; b < radix_detail::k_bucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]  = sum;
            sum += c;
        }

        for (UInt32 i = 0; i < n; ++i)
        {
            const uint64_t k   = inK[i];
            const UInt32   b   = static_cast<UInt32>((k >> shift) & radix_detail::k_mask);
            UInt32   dst = hist[b]++;
            outK[dst]    = k;
            outV[dst]    = inV[i];
        }

        {
            const auto tmp = inK;
            inK      = outK;
            outK     = tmp;
        }
        {
            auto tmp = inV;
            inV      = outV;
            outV     = tmp;
        }
    }

    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(uint64_t) * n);
        for (UInt32 i = 0; i < n; ++i)
            values[i] = tempValues[i];
    }
}

}   // namespace jam