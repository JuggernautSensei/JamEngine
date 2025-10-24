#pragma once

namespace jam
{

/// @brief 시간 단위
enum class eTimeUnit : UInt8
{
    Nano = 0,
    Micro,
    Milli,
    Sec,
};

/// @brief 나노 초 단위를 다른 시간 단위로 변환
/// @tparam TimeUnit 변환할 시간 단위
/// @param _nano 나노 초 단위 시간
/// @param _timeUnit 변환할 시간 단위
/// @return 변환된 시간
JAM_NODISCARD inline float ConvertNsTo(const UInt64 _nano, const eTimeUnit _timeUnit)
{
    switch (_timeUnit)
    {
        case eTimeUnit::Nano: return static_cast<float>(_nano);
        case eTimeUnit::Micro: return static_cast<float>(_nano) * 1e-3f;
        case eTimeUnit::Milli: return static_cast<float>(_nano) * 1e-6f;
        case eTimeUnit::Sec: return static_cast<float>(_nano) * 1e-9f;
        default:
            JAM_ASSERT(0, "Unknown time unit");
            return 0.f;
    }
}

/// @brief 나노 초 단위를 다른 시간 단위로 변환
/// @tparam TimeUnit 변환할 시간 단위
/// @param _nano 나노 초 단위 시간
/// @param _timeUnit 변환할 시간 단위
/// @return 변환된 시간
JAM_NODISCARD inline double ConvertNsToDouble(const UInt64 _nano, const eTimeUnit _timeUnit)
{
    switch (_timeUnit)
    {
        case eTimeUnit::Nano: return static_cast<double>(_nano);
        case eTimeUnit::Micro: return static_cast<double>(_nano) * 1e-3;
        case eTimeUnit::Milli: return static_cast<double>(_nano) * 1e-6;
        case eTimeUnit::Sec: return static_cast<double>(_nano) * 1e-9;
        default:
            JAM_ASSERT(0, "Unknown time unit");
            return 0;
    }
}

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
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Init() first.");
        const auto endCounter = std::chrono::steady_clock::now();
        m_totalElapsedTimeNs += std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_bRunning = false;
    }

    void Lap()
    {
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Init() first.");
        const auto endCounter = std::chrono::steady_clock::now();
        m_intervalTimeNs      = std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_startCounter        = endCounter;
        m_totalElapsedTimeNs += m_intervalTimeNs;
    }

    void Reset()
    {
        m_totalElapsedTimeNs = 0;
        m_intervalTimeNs     = 0;
        m_bRunning           = false;
    }

    JAM_NODISCARD bool IsRunning() const
    {
        return m_bRunning;
    }

    JAM_NODISCARD UInt64 GetTotalElapsedNs() const
    {
        return m_totalElapsedTimeNs;
    }

    JAM_NODISCARD float GetTotalElapsed(const eTimeUnit _timeUnit) const
    {
        return ConvertNsTo(m_totalElapsedTimeNs, _timeUnit);
    }

    JAM_NODISCARD double GetTotalElapsedDouble(const eTimeUnit _timeUnit) const
    {
        return ConvertNsToDouble(m_totalElapsedTimeNs, _timeUnit);
    }

    JAM_NODISCARD Int64 GetIntervalNs() const
    {
        return m_intervalTimeNs;
    }

    JAM_NODISCARD float GetInterval(const eTimeUnit _timeUnit) const
    {
        return ConvertNsTo(m_intervalTimeNs, _timeUnit);
    }

    JAM_NODISCARD double GetIntervalDouble(const eTimeUnit _timeUnit) const
    {
        return ConvertNsToDouble(m_intervalTimeNs, _timeUnit);
    }

protected:
    std::chrono::steady_clock::time_point m_startCounter;
    Int64                                 m_totalElapsedTimeNs = 0;
    Int64                                 m_intervalTimeNs     = 0;
    bool                                  m_bRunning           = false;
};

class TickTimer
{
public:
    void Start()   // 0 -> unlimited frame rate
    {
        m_timer.Start();
    }

    void Reset()
    {
        m_timer.Reset();
        m_deltaNs = 0;
    }

    void Tick()
    {
        JAM_ASSERT(m_timer.IsRunning(), "Timer is not running. Call Init() first.");
        m_timer.Lap();
        m_deltaNs = m_timer.GetIntervalNs();
    }

    JAM_NODISCARD Int64 GetDeltaNs() const
    {
        return m_deltaNs;
    }

    JAM_NODISCARD float GetDelta(const eTimeUnit _timeUnit) const
    {
        return ConvertNsTo(m_deltaNs, _timeUnit);
    }

    JAM_NODISCARD double GetDeltaDouble(const eTimeUnit _timeUnit) const
    {
        return ConvertNsToDouble(m_deltaNs, _timeUnit);
    }

private:
    Timer m_timer   = {};
    Int64 m_deltaNs = 0;   // delta time in seconds
};

// time utilities
struct TimeStamp
{
    static JAM_NODISCARD TimeStamp Current();

    Int32 year        = 0;
    Int32 month       = 0;
    Int32 day         = 0;
    Int32 hour        = 0;
    Int32 minute      = 0;
    Int32 second      = 0;
    Int32 millisecond = 0;
};

class ScopedTimer : private Timer
{
public:
    explicit ScopedTimer(const std::string_view _name, const eTimeUnit _timeUnit = eTimeUnit::Milli)
        : m_name(_name)
        , m_timeUnit(_timeUnit)
    {
        Log::Trace(R"(========== scoped timer start - "{}" ==========")", m_name);
        Start();
    }

    ~ScopedTimer()
    {
        Stop();
        float elapsedTime = GetTotalElapsed(m_timeUnit);
        Log::Trace(R"(========== scoped timer end - "{}" - duration: {} {} ==========)", m_name, elapsedTime, EnumToString(m_timeUnit));
    }

    ScopedTimer(const ScopedTimer&)                = delete;
    ScopedTimer(ScopedTimer&&) noexcept            = delete;
    ScopedTimer& operator=(const ScopedTimer&)     = delete;
    ScopedTimer& operator=(ScopedTimer&&) noexcept = delete;

private:
    std::string m_name;
    eTimeUnit   m_timeUnit = eTimeUnit::Milli;
};

}   // namespace jam