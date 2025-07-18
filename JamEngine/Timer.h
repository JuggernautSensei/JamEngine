#pragma once

namespace jam
{

class Timer
{
public:
    void Start()
    {
        JAM_ASSERT(!m_bRunning, "Timer is already running. Call Stop() first.");
        m_startCounter = std::chrono::steady_clock::now();
        m_bRunning     = true;
    }

    void Stop()
    {
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Initialize() first.");
        const auto endCounter = std::chrono::steady_clock::now();
        m_totalElapsedTimeNs += std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_bRunning = false;
    }

    void Lab()
    {
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Initialize() first.");
        const auto endCounter = std::chrono::steady_clock::now();
        m_intervalTimeNs      = std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_totalElapsedTimeNs += m_intervalTimeNs;
        m_startCounter = endCounter;
    }

    void Reset()
    {
        m_totalElapsedTimeNs = 0;
        m_intervalTimeNs     = 0;
        m_bRunning           = false;
    }

    NODISCARD bool IsRunning() const { return m_bRunning; }

    NODISCARD Int64 GetTotalElapsedNs() const { return m_totalElapsedTimeNs; }
    NODISCARD Int64 GetIntervalNs() const { return m_intervalTimeNs; }

protected:
    std::chrono::steady_clock::time_point m_startCounter;
    Int64                                 m_totalElapsedTimeNs = 0;
    Int64                                 m_intervalTimeNs     = 0;
    bool                                  m_bRunning           = false;
};

class ScopedTimer : private Timer
{
public:
    explicit ScopedTimer(const std::string_view _name = "")
        : m_name(_name)
    {
        Start();
        Log::Trace(R"(scoped timer start - "{}")", std::string(m_name));
    }

    ~ScopedTimer()
    {
        Stop();
        float durationMs = GetTotalElapsedNs() * 1e-6f;   // convert nanoseconds to milliseconds
        Log::Trace(R"(scoped timer end - "{}" - duration: {:.3f} ms)", std::string(m_name), durationMs);
    }

    ScopedTimer(const ScopedTimer&)                = default;
    ScopedTimer(ScopedTimer&&) noexcept            = default;
    ScopedTimer& operator=(const ScopedTimer&)     = default;
    ScopedTimer& operator=(ScopedTimer&&) noexcept = default;

private:
    std::string_view m_name;
};

class TickTimer
{
public:
    void Start(const float _targetFrameRate)   // 0 -> unlimited frame rate
    {
        m_timer.Reset();
        m_targetDeltaNs = _targetFrameRate <= 0.f ? 0 : static_cast<Int64>(1e9 / _targetFrameRate);
        m_timer.Start();
        m_counterNs = 0;
    }

    NODISCARD bool Tick()
    {
        JAM_ASSERT(m_timer.IsRunning(), "Timer is not running. Call Initialize() first.");
        m_timer.Lab();

        // unlimited frame rate
        if (m_targetDeltaNs == 0)
        {
            m_deltaNs   = m_timer.GetIntervalNs();
            m_counterNs = 0;
            return true;
        }

        // accumulate time
        m_counterNs += m_timer.GetIntervalNs();

        // check if enough time has passed for the next tick
        if (m_counterNs >= m_targetDeltaNs)
        {
            m_deltaNs   = m_counterNs;
            m_counterNs = m_counterNs - m_targetDeltaNs;   // accumulate the remaining time
            return true;                                   // tick successful
        }

        return false;   // tick not ready
    }

    NODISCARD float GetDeltaSec() const { return static_cast<float>(m_deltaNs * 1e-9); }
    NODISCARD float GetTargetFrameRate() const { return m_targetDeltaNs <= 0 ? 0.f : static_cast<float>(1e9 / m_targetDeltaNs); }

private:
    Timer m_timer         = {};
    Int64 m_deltaNs       = 0;                              // delta time in seconds
    Int64 m_targetDeltaNs = static_cast<Int64>(1e9 / 60);   // target delta time in seconds (1.0f / targetFrameRate)
    Int64 m_counterNs     = 0;                              // accumulated time counter
};

// time utilities
struct TimeStamp
{
    static NODISCARD TimeStamp Create();

    Int32 year        = 0;
    Int32 month       = 0;
    Int32 day         = 0;
    Int32 hour        = 0;
    Int32 minute      = 0;
    Int32 second      = 0;
    Int32 millisecond = 0;
};

}   // namespace jam