#pragma once
#include "FVector.h"

namespace jam
{

class ConsolePanel
{
    using eLogLevel = spdlog::level::level_enum;

    enum eLogLevelFilterFlags_ : int
    {
        eLogLevelFilterFlags_None     = 0,
        eLogLevelFilterFlags_Trace    = 1 << EnumToInt(eLogLevel::trace),
        eLogLevelFilterFlags_Debug    = 1 << EnumToInt(eLogLevel::debug),
        eLogLevelFilterFlags_Info     = 1 << EnumToInt(eLogLevel::info),
        eLogLevelFilterFlags_Warn     = 1 << EnumToInt(eLogLevel::warn),
        eLogLevelFilterFlags_Error    = 1 << EnumToInt(eLogLevel::err),
        eLogLevelFilterFlags_Critical = 1 << EnumToInt(eLogLevel::critical),
        eLogLevelFilterFlags_Off      = 1 << EnumToInt(eLogLevel::off),
        eLogLevelFilterFlags_All      = eLogLevelFilterFlags_Trace | eLogLevelFilterFlags_Debug | eLogLevelFilterFlags_Info | eLogLevelFilterFlags_Warn | eLogLevelFilterFlags_Error | eLogLevelFilterFlags_Critical | eLogLevelFilterFlags_Off
    };
    static_assert(EnumCount<eLogLevelFilterFlags_>() == EnumCount<eLogLevel>() + 1, "eLogLevelFilterFlags_ must have the same count as eLogLevel plus one for None.");
    using eLogLevelFilterFlags = std::underlying_type_t<eLogLevelFilterFlags_>;

public:
    void Show(bool* _pOpenOrNull);

private:
    void CollectLogEntries_();

    // filter
    ImGuiTextFilter      m_textFilter;
    eLogLevelFilterFlags m_filterFlags = eLogLevelFilterFlags_All;

    // for log dirty check
    MemorySinkMT::Container::const_iterator m_logContainerEndIt;
    MemorySinkMT::Container::const_iterator m_logContainerBeginIt;

    // collection
    fixed_vector<const LogItem*, log::k_memorySinkLogStoreCapacity> m_filteredLogCollection;   // 렌더링할 로그 항목들 수집

    // misc
    bool m_bAutoScroll = true;   // 자동 스크롤 여부

    // 로그 레벨별 텍스트 색상
    constexpr static ImVec4 k_logColors[EnumCount<spdlog::level::level_enum>()] = {
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f),   // trace, white
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),   // debug, green
        ImVec4(0.3f, 0.6f, 1.0f, 1.0f),   // info, blue
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f),   // warn, yellow
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),   // error, red
        ImVec4(1.0f, 0.0f, 1.0f, 1.0f),   // critical, purple
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f),   // off, gray
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f)    // none, dummy color
    };
};

}   // namespace jam
