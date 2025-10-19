#include "pch.h"

#include "RadixSort.h"

namespace jam
{

void RadixSort(UInt32* keys, UInt32* tempKeys, const UInt32 n)
{
    if (n <= 1) 
        return;

    UInt32* in  = keys;
    UInt32* out = tempKeys;

    UInt32 hist[radix_detail::k_bucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 3; ++pass, shift = static_cast<UInt16>(shift + radix_detail::k_bits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            UInt32 prev = in[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                UInt32 k = in[i];
                ++hist[(k >> shift) & radix_detail::k_mask];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted) break;

        // prefix-sum
        UInt32 sum = 0;
        for (UInt32 b = 0; b < radix_detail::k_bucketCount; ++b)
        {
            UInt32 c = hist[b];
            hist[b]    = sum;
            sum += c;
        }

        // stable scatter
        for (UInt32 i = 0; i < n; ++i)
        {
            UInt32 k     = in[i];
            UInt32 b     = (k >> shift) & radix_detail::k_mask;
            out[hist[b]++] = k;
        }

        // swap buffers
        UInt32* tmp = in;
        in            = out;
        out           = tmp;
    }

    // 홀수 패스(또는 early break)일 수 있으므로, 결과가 temp에 있는 경우 복사
    if ((pass & 1u) != 0)
    {
        std::memcpy(keys, tempKeys, sizeof(UInt32) * n);
    }
}

void RadixSort(uint64_t* keys, uint64_t* tempKeys, const UInt32 n)
{
    if (n <= 1) 
        return;

    uint64_t* in  = keys;
    uint64_t* out = tempKeys;

    UInt32 hist[radix_detail::k_bucketCount];
    UInt16 shift = 0;
    UInt32 pass  = 0;
    for (; pass < 6; ++pass, shift = static_cast<UInt16>(shift + radix_detail::k_bits))
    {
        std::memset(hist, 0, sizeof(hist));

        bool alreadySorted = true;
        {
            uint64_t prev = in[0];
            for (UInt32 i = 0; i < n; ++i)
            {
                uint64_t k = in[i];
                ++hist[static_cast<UInt32>((k >> shift) & radix_detail::k_mask)];
                alreadySorted &= (prev <= k);
                prev = k;
            }
        }
        if (alreadySorted) break;

        UInt32 sum = 0;
        for (UInt32 b = 0; b < radix_detail::k_bucketCount; ++b)
        {
            UInt32 c = hist[b];
            hist[b]  = sum;
            sum += c;
        }

        for (UInt32 i = 0; i < n; ++i)
        {
            uint64_t k     = in[i];
            UInt32   b     = static_cast<UInt32>((k >> shift) & radix_detail::k_mask);
            out[hist[b]++] = k;
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

}