#include "pch.h"

#include "Timer.h"

namespace jam
{

double NsTo(const Int64 _ns, const eSIPrefix _unit)
{
    switch (_unit)
    {
        case eSIPrefix::Nano: return static_cast<double>(_ns);
        case eSIPrefix::Micro: return static_cast<double>(_ns) * 1e-3;
        case eSIPrefix::Milli: return static_cast<double>(_ns) * 1e-6;
        case eSIPrefix::Sec: return static_cast<double>(_ns) * 1e-9;
        default:
            JAM_ASSERT(0, "Unknown time unit");
            return 0.f;
    }
}

void Timer::Start()
{
    JAM_ASSERT(!m_bRunning, "Timer is already running. Call Stop() first.");
    m_startCounter = std::chrono::steady_clock::now();
    m_bRunning     = true;
}

void Timer::Stop()
{
    JAM_ASSERT(m_bRunning, "Timer is not running. Call Init() first.");
    const auto counter = std::chrono::steady_clock::now();
    m_totalNs += std::chrono::duration_cast<std::chrono::nanoseconds>(counter - m_startCounter).count();
    m_bRunning = false;
}

void Timer::Lap()
{
    JAM_ASSERT(m_bRunning, "Timer is not running. Call Init() first.");
    const auto endCounter = std::chrono::steady_clock::now();
    m_intervalNs          = std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
    m_startCounter        = endCounter;
    m_totalNs += m_intervalNs;
}

void Timer::Reset()
{
    m_totalNs    = 0;
    m_intervalNs = 0;
    m_bRunning   = false;
}

void Timer::Restart()
{
    Reset();
    Start();
}

bool Timer::IsRunning() const
{
    return m_bRunning;
}

double Timer::GetTotal(const eSIPrefix _unit) const
{
    return NsTo(m_totalNs, _unit);
}

double Timer::GetInterval(const eSIPrefix _unit) const
{
    return NsTo(m_intervalNs, _unit);
}

Int64 Timer::GetTotalNs() const
{
    return m_totalNs;
}

Int64 Timer::GetIntervalNs() const
{
    return m_intervalNs;
}

void TickTimer::Start()
{
    m_timer.Start();
}

void TickTimer::Reset()
{
    m_timer.Reset();
    m_deltaNs = 0;
}

void TickTimer::Tick()
{
    JAM_ASSERT(m_timer.IsRunning(), "Timer is not running. Call Init() first.");
    m_timer.Lap();
    m_deltaNs = m_timer.GetIntervalNs();
}

Int64 TickTimer::GetDeltaNs() const
{
    return m_deltaNs;
}

double TickTimer::GetDelta(const eSIPrefix _unit) const
{
    return NsTo(m_deltaNs, _unit);
}

TimeStamp TimeStamp::Current()
{
    using namespace std::chrono;
    const auto        now = system_clock::now();
    const auto        ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t t   = system_clock::to_time_t(now);
    std::tm           tm;
    TimeStamp         stamp;

    if (localtime_s(&tm, &t) == 0)
    {
        stamp.year        = 1900 + tm.tm_year;
        stamp.month       = 1 + tm.tm_mon;
        stamp.day         = tm.tm_mday;
        stamp.hour        = tm.tm_hour;
        stamp.minute      = tm.tm_min;
        stamp.second      = tm.tm_sec;
        stamp.millisecond = static_cast<int>(ms.count());
    }

    return stamp;
}

ScopedTimer::ScopedTimer(const std::string_view _name, const eSIPrefix _unit)
    : m_name(_name)
    , m_unit(_unit)
{
    Log::Trace(R"([TIMER START] {})", m_name);
    Start();
}

ScopedTimer::~ScopedTimer()
{
    Stop();
    double elapsedTime = GetTotal(m_unit);
    Log::Trace(R"([TIMER END] {} [{:3f} {}])", m_name, elapsedTime, StringFrom(m_unit));
}

}   // namespace jam