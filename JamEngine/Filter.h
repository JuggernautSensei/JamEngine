#pragma once

namespace Jam
{

void MeanFilter1D(Span<float> _outKernel);
void GaussianFilter1D(Span<float> _outKernel, float _sigma);

}