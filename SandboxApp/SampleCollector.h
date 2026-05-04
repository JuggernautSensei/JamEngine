#pragma once

class SampleCollector
{
public:
    void PushSample(float _cpuSec);

    const float*        GetCpuTimeSamples() const;
    [[nodiscard]] float GetAverage() const;
    [[nodiscard]] Int32 GetSampleCount() const;
    [[nodiscard]] Int32 GetOffset() const;

    constexpr static Int32 kMaxSampleCount = 100;

private:
    Array<float, kMaxSampleCount> m_samples = {};
    Int32                         m_offset  = 0;
    Int32                         m_num     = 0;
    float                         m_sum     = 0.f;
    float                         m_avg     = 0.f;
};
