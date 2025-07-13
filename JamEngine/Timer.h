#pragma once

namespace jam
{

class Timer
{
public:
    void Start()
    {
        m_startCounter = std::chrono::steady_clock::now();
        m_bRunning     = true;
    }

    void End()
    {
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Start() first.");
        auto endCounter = std::chrono::steady_clock::now();
        auto duration   = std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_durationSec += static_cast<float>(duration) / 1e9f;   // ns -> sec
        m_bRunning = false;
    }

    void Tick()
    {
        JAM_ASSERT(m_bRunning, "Timer is not running. Call Start() first.");
        auto endCounter = std::chrono::steady_clock::now();
        auto duration   = std::chrono::duration_cast<std::chrono::nanoseconds>(endCounter - m_startCounter).count();
        m_durationSec += static_cast<float>(duration) / 1e9f;   // ns -> sec
        m_startCounter = endCounter;
    }

    void Reset()
    {
        m_durationSec = 0.0f;
        m_bRunning    = false;
    }

    NODISCARD bool IsRunning() const
    {
        return m_bRunning;
    }

    NODISCARD float GetDurationSec() const
    {
        return m_durationSec;
    }

protected:
    std::chrono::steady_clock::time_point m_startCounter;
    float                                 m_durationSec = 0.0f;
    bool                                  m_bRunning    = false;
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
        End();
        float durationSec = GetDurationSec();
        Log::Trace(R"(scoped timer end - "{}" - duration: {:.3f} sec)", std::string(m_name), durationSec);
    }

    ScopedTimer(const ScopedTimer&)                = default;
    ScopedTimer(ScopedTimer&&) noexcept            = default;
    ScopedTimer& operator=(const ScopedTimer&)     = default;
    ScopedTimer& operator=(ScopedTimer&&) noexcept = default;

private:
    std::string_view m_name;
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