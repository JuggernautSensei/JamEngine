#include "pch.h"
#include "Filter.h"

namespace Jam
{

void CalcMeanFilter1D(
    const Span<float> _outKernel)
{
    const SizeT len = _outKernel.size();
    JAM_ASSERT(len % 2 == 1, "Mean filter kernel size should be odd.");

    const float v = 1.f / static_cast<float>(len);
    for (SizeT i = 0; i < len; ++i)
    {
        _outKernel[i] = v;
    }
}

void CalcGaussianFilter1D(
    const Span<float> _outKernel,
    const float       _sigma)
{
    const SizeT len = _outKernel.size();
    JAM_ASSERT(len % 2 == 1, "Gaussian filter kernel size should be odd.");

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