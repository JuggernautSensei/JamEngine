#pragma once

namespace Jam::RadixSort
{

namespace Details
{
    constexpr UInt32 kBits        = 11;
    constexpr UInt32 kBucketCount = 1u << kBits;
    constexpr UInt32 kMask        = kBucketCount - 1u;
}   // namespace Details

// 32 bit key
void Sort(UInt32* keys, UInt32* tempKeys, UInt32 n);

// 32 bit key + values
template<class T>
void Sort(UInt32* keys, UInt32* tempKeys, T* values, T* tempValues, const UInt32 n)
{
    static_assert(std::is_trivially_move_assignable_v<T>, "Value type must be trivially move assignable.");
    if (n <= 1)
    {
        return;
    }

    UInt32* inK  = keys;
    UInt32* outK = tempKeys;
    T*      inV  = values;
    T*      outV = tempValues;

    UInt32 hist[Details::kBucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 3; ++pass, shift = static_cast<UInt16>(shift + Details::kBits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            UInt32 prev = inK[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const UInt32 k = inK[i];
                ++hist[(k >> shift) & Details::kMask];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted)
        {
            break;
        }

        // prefix-sum
        UInt32 sum = 0;
        for (UInt32 b = 0; b < Details::kBucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]        = sum;
            sum += c;
        }

        // stable scatter (key+value)
        for (UInt32 i = 0; i < n; ++i)
        {
            const UInt32 k   = inK[i];
            const UInt32 b   = (k >> shift) & Details::kMask;
            UInt32       dst = hist[b]++;
            outK[dst]        = k;
            outV[dst]        = inV[i];
        }

        // swap buffers
        {
            const auto tmp = inK;
            inK            = outK;
            outK           = tmp;
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
void Sort(UInt64* keys, UInt64* tempKeys, UInt32 n);

// 64-bit key + values
template<class T>
void Sort(UInt64* keys, UInt64* tempKeys, T* values, T* tempValues, const UInt32 n)
{
    static_assert(std::is_trivially_move_assignable_v<T>, "Value type must be trivially move assignable.");
    if (n <= 1) return;

    UInt64* inK  = keys;
    UInt64* outK = tempKeys;
    T*      inV  = values;
    T*      outV = tempValues;

    UInt32 hist[Details::kBucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 6; ++pass, shift = static_cast<UInt16>(shift + Details::kBits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            UInt64 prev = inK[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const UInt64 k = inK[i];
                ++hist[static_cast<UInt32>((k >> shift) & Details::kMask)];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted)
            break;

        UInt32 sum = 0;
        for (UInt32 b = 0; b < Details::kBucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]        = sum;
            sum += c;
        }

        for (UInt32 i = 0; i < n; ++i)
        {
            const UInt64 k   = inK[i];
            const UInt32 b   = static_cast<UInt32>((k >> shift) & Details::kMask);
            UInt32       dst = hist[b]++;
            outK[dst]        = k;
            outV[dst]        = inV[i];
        }

        {
            const auto tmp = inK;
            inK            = outK;
            outK           = tmp;
        }
        {
            auto tmp = inV;
            inV      = outV;
            outV     = tmp;
        }
    }

    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(UInt64) * n);
        for (UInt32 i = 0; i < n; ++i)
        {
            values[i] = tempValues[i];
        }
    }
}

}   // namespace Jam::RadixSort
