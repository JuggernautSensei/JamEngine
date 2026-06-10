#include "pch.h"
#include "SampleCollector.h"

void SampleCollector::PushSample(
    const float _cpuSec)
{
    if (m_num < kMaxSampleCount)
    {
        m_samples[m_num] = _cpuSec;
        ++m_num;
    }
    else
    {
        const Int32 nextOffset = (m_offset + 1) % kMaxSampleCount;
        const float oldest     = m_samples[nextOffset];
        m_sum -= oldest;

        m_offset            = nextOffset;
        m_samples[m_offset] = _cpuSec;
    }

    m_sum += _cpuSec;
    m_avg = static_cast<float>(m_sum) / static_cast<float>(m_num);
}

const float* SampleCollector::GetCpuTimeSamples() const
{
    return m_samples.data();
}

float SampleCollector::GetAverage() const
{
    return m_avg;
}

Int32 SampleCollector::GetSampleCount() const
{
    return m_num;
}

Int32 SampleCollector::GetOffset() const
{
    return m_offset;
}

void SampleCollector::Reset()
{
    m_samples.fill(0);
    m_offset = 0;
    m_num    = 0;
    m_sum    = 0.f;
    m_avg    = 0.f;
}
