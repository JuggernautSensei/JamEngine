#include "pch.h"

#include "RadixSort.h"

namespace Jam::RadixSort
{

void Sort(UInt32* keys, UInt32* tempKeys, const UInt32 n)
{
    if (n <= 1)
    {
        return;
    }

    UInt32* in  = keys;
    UInt32* out = tempKeys;

    UInt32 hist[Details::kBucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 3; ++pass, shift = static_cast<UInt16>(shift + Details::kBits))
    {
        std::memcpy(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            UInt32 prev = in[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const UInt32 k = in[i];
                ++hist[(k >> shift) & Details::kMask];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted) break;

        // prefix-sum
        UInt32 sum = 0;
        for (UInt32 b = 0; b < Details::kBucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]        = sum;
            sum += c;
        }

        // stable scatter
        for (UInt32 i = 0; i < n; ++i)
        {
            const UInt32 k = in[i];
            const UInt32 b = (k >> shift) & Details::kMask;
            out[hist[b]++] = k;
        }

        // swap buffers
        UInt32* tmp = in;
        in          = out;
        out         = tmp;
    }

    // 홀수 패스(또는 early break)일 수 있으므로, 결과가 temp에 있는 경우 복사
    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(UInt32) * n);
    }
}

void Sort(uint64_t* keys, uint64_t* tempKeys, const UInt32 n)
{
    if (n <= 1)
        return;

    uint64_t* in  = keys;
    uint64_t* out = tempKeys;

    UInt32 hist[Details::kBucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 6; ++pass, shift = static_cast<UInt16>(shift + Details::kBits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            uint64_t prev = in[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                const uint64_t k = in[i];
                ++hist[static_cast<UInt32>((k >> shift) & Details::kMask)];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted) break;

        UInt32 sum = 0;
        for (UInt32 b = 0; b < Details::kBucketCount; ++b)
        {
            const UInt32 c = hist[b];
            hist[b]        = sum;
            sum += c;
        }

        for (UInt32 i = 0; i < n; ++i)
        {
            const uint64_t k = in[i];
            const UInt32   b = static_cast<UInt32>((k >> shift) & Details::kMask);
            out[hist[b]++]   = k;
        }

        uint64_t* tmp = in;
        in            = out;
        out           = tmp;
    }

    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(uint64_t) * n);
    }
}

}   // namespace Jam