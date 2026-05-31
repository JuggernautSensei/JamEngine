#pragma once

namespace Jam
{

void CalcMeanFilter1D(Span<float> _outKernel);
void CalcGaussianFilter1D(Span<float> _outKernel, float _sigma);

}   // namespace Jam