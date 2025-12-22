#pragma once

namespace jam
{

enum class eSIPrefix : UInt8
{
    Nano = 0,
    Micro,
    Milli,
    Sec,
};

[[nodiscard]] double NsTo(Int64 _ns, eSIPrefix _unit);

class Timer
{
public:
    void Start();
    void Stop();
    void Lap();
    void Reset();
    void Restart();

    [[nodiscard]] bool   IsRunning() const;
    [[nodiscard]] double GetTotal(eSIPrefix _unit) const;
    [[nodiscard]] double GetInterval(eSIPrefix _unit) const;
    [[nodiscard]] Int64  GetTotalNs() const;
    [[nodiscard]] Int64  GetIntervalNs() const;

protected:
    std::chrono::steady_clock::time_point m_startCounter;

    Int64 m_totalNs    = 0;
    Int64 m_intervalNs = 0;
    bool  m_bRunning   = false;
};

class TickTimer
{
public:
    void Start();   // 0 -> unlimited frame rate
    void Reset();
    void Tick();

    [[nodiscard]] Int64  GetDeltaNs() const;
    [[nodiscard]] double GetDelta(eSIPrefix _unit) const;

private:
    Timer m_timer   = {};
    Int64 m_deltaNs = 0;   // delta time in seconds
};

class ScopedTimer : private Timer
{
public:
    explicit ScopedTimer(StringView _name, eSIPrefix _unit = eSIPrefix::Milli);
    ~ScopedTimer();

    ScopedTimer(const ScopedTimer&)                = delete;
    ScopedTimer(ScopedTimer&&) noexcept            = delete;
    ScopedTimer& operator=(const ScopedTimer&)     = delete;
    ScopedTimer& operator=(ScopedTimer&&) noexcept = delete;

private:
    std::string m_name;
    eSIPrefix   m_unit = eSIPrefix::Milli;
};

// time utilities
struct TimeStamp
{
    static [[nodiscard]] TimeStamp Current();

    Int32 year        = 0;
    Int32 month       = 0;
    Int32 day         = 0;
    Int32 hour        = 0;
    Int32 minute      = 0;
    Int32 second      = 0;
    Int32 millisecond = 0;
};

}   // namespace jam