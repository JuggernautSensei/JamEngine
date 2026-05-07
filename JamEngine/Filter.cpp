#include "pch.h"
#include "Filter.h"

namespace Jam
{

void MeanFilter1D(
    const Span<float> _outKernel)
{
    const SizeT len = _outKernel.size();
    const float v   = 1.f / static_cast<float>(len);
    for (SizeT i = 0; i < len; ++i)
    {
        _outKernel[i] = v;
    }
}

void GaussianFilter1D(
    const Span<float> _outKernel,
    const float       _sigma)
{
    const SizeT len     = _outKernel.size();
    const SizeT halfLen = len / 2;
    for (SizeT i = 0; i < len; ++i)
    {
        const float x    = static_cast<float>(i) - static_cast<float>(halfLen);
        float       exp  = Exp(-(x * x) / (2.f * _sigma * _sigma));
        float       root = 1.f / (_sigma * Sqrt(2.f * kPi));
        _outKernel[i]    = root * exp;
    }

    // normalize
    float sum = 0.f;
    for (SizeT i = 0; i < len; ++i)
    {
        sum += _outKernel[i];
    }

    float invSum = 1.f / sum;
    for (SizeT i = 0; i < len; ++i)
    {
        _outKernel[i] *= invSum;
    }
}

}   // namespace Jam