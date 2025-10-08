#pragma once
#include "fixed_circular_queue.h"
#include "LogBase.h"
#include <spdlog/sinks/base_sink.h>

namespace jam
{

struct LogItem
{
    std::string log;
    eLogLevel   level;
};

template<typename Mutex, size_t Size = 8192>
class MemorySink final : public spdlog::sinks::base_sink<Mutex>
{
public:
    using ContainerT = fixed_ciruclar_queue<LogItem, Size, eFixedCircularQueueOverflowPolicy::PopFront>;

    JAM_NODISCARD decltype(auto) GetBeginIter() const
    {
        return m_logContainer.cbegin();
    }

    JAM_NODISCARD decltype(auto) GetEndIter() const
    {
        return m_logContainer.cend();
    }

    JAM_NODISCARD size_t GetCount() const
    {
        return m_logContainer.size();
    }

    void Clear()
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        m_logContainer.clear();
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        using namespace spdlog;
        using namespace spdlog::sinks;
        using namespace spdlog::details;

        constexpr eLogLevel k_logLevelTable[spdlog::level::level_enum::n_levels] = {
            eLogLevel::Trace,
            eLogLevel::Trace,
            eLogLevel::Info,
            eLogLevel::Warn,
            eLogLevel::Error,
            eLogLevel::Critical,
            eLogLevel::Off
        };

        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        m_logContainer.emplace_back(SPDLOG_BUF_TO_STRING(formatted), k_logLevelTable[msg.level]);
    }

    void flush_() override
    {
    }

    ContainerT m_logContainer;
};

using MemorySinkMT = MemorySink<std::mutex>;
using MemorySinkST = MemorySink<spdlog::details::null_mutex>;

}   // namespace jam
