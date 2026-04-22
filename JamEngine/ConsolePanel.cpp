#include "pch.h"

#include "ConsolePanel.h"

#include "Application.h"
#include "EditorUtilities.h"

namespace jam
{

void ConsolePanel::Show(bool* _pOpenOrNull)
{
    if (ImGui::Begin("console", _pOpenOrNull))
    {
        bool bNeedCollect = false;   // 로그 항목을 수집해야 하는지
        bool bNeedScroll  = false;   // 맨 밑으로 로그 스크롤

        constexpr float kButtonWidth = 50.f;

        // 텍스트 필터
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("filter: ");
            ImGui::SameLine();
            float filterBoxWidth = ImGui::GetContentRegionAvail().x - kButtonWidth - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().WindowPadding.x;   // 필터 박스의 너비 계산
            bNeedCollect |= m_textFilter.Draw("##filter", filterBoxWidth);                                                                                   // 수동으로 수집 여부를 설정함
        }

        // 옵션 버튼
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(kButtonWidth);
            ImGui::Button("options");
        }

        // 옵션
        if (ImGui::BeginPopupContextItem(NULL, ImGuiMouseButton_Left))
        {

#ifdef JAM_DEBUG
            // [디버그 용] 디버그 로그 생성
            if (ImGui::BeginMenu("create debug log"))
            {
                static int s_logCount = 5;
                ImGui::Text("number of logs to generate");
                ImGui::SliderInt("numberOfLogs", &s_logCount, 1, 100);
                editor::SetNextItemWidthToAvailableWidth();
                if (ImGui::Button("generate"))
                {
                    for (int i = 0; i < s_logCount; ++i)
                    {
                        using LoggingFunc                 = void (*)(const std::string&);
                        constexpr LoggingFunc k_Logging[] = { log::Trace<std::string>, log::Debug<std::string>, log::Info<std::string>, log::Warn<std::string>, log::Error<std::string>, log::Critical<std::string> };
                        k_Logging[Random::Generate(0ull, std::size(k_Logging))](fmt::format("this is a debug log log {}: {}", i + 1, Random::Generate(0, 1000)));
                    }
                }

                ImGui::EndMenu();
            }
#endif

            // 클립보드에 복사
            if (ImGui::MenuItem("copy to clipboard"))
            {
                constexpr std::string_view kFormat = "[{}] {}\n";
                std::string                buf;   // 문자열 버퍼

                // 재할당 방지를 위한 버퍼 크기 계산
                size_t bufSize = 0;
                for (const LogItem* pLog: m_filteredLogCollection)
                {
                    bufSize += kFormat.size() + EnumToString(pLog->level).size() + pLog->log.size();
                }
                buf.reserve(bufSize);   // 버퍼 크기 예약

                // 필터링된 로그 항목들을 문자열로 변환
                for (const LogItem* pLog: m_filteredLogCollection)
                {
                    buf += fmt::format(kFormat, EnumToString(pLog->level), pLog->log);
                }
                ImGui::SetClipboardText(buf.c_str());   // 클립보드에 복사
            }

            // 로그 초기화
            if (ImGui::MenuItem("clear log"))
            {
                log::GetMemorySink()->Clear();
            }

            // 로그 설정
            if (ImGui::BeginMenu("event logging filter"))
            {
                eEventCategoryFlags flags = Application::GetInstance().GetEventLoggingFilter();

                bool bDirty = false;
                bDirty |= ImGui::CheckboxFlags("window", &flags, eEventCategoryFlags_Window);
                bDirty |= ImGui::CheckboxFlags("input", &flags, eEventCategoryFlags_Input);
                bDirty |= ImGui::CheckboxFlags("renderer", &flags, eEventCategoryFlags_Graphics);
                bDirty |= ImGui::CheckboxFlags("file system", &flags, eEventCategoryFlags_FileSystem);
                bDirty |= ImGui::CheckboxFlags("scene layer", &flags, eEventCategoryFlags_SceneLayer);
                bDirty |= ImGui::CheckboxFlags("asset manager", &flags, eEventCategoryFlags_AssetManager);

                if (bDirty)   // 필터가 변경되었으면 적용
                {
                    Application::GetInstance().SetEventLoggingFilter(flags);
                }
                ImGui::EndMenu();
            }

            // 오토 스크롤
            if (ImGui::MenuItem("auto scroll", nullptr, m_bAutoScroll))
            {
                m_bAutoScroll = !m_bAutoScroll;   // 오토 스크롤 토글
            }

            ImGui::EndPopup();
        }

        // 로그 레벨 필터링
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("log level: ");

            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("all", &m_filterFlags, eLogLevelFilterFlags_All);   // 전체 선택
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("trace", &m_filterFlags, eLogLevelFilterFlags_Trace);
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("debug", &m_filterFlags, eLogLevelFilterFlags_Debug);
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("info", &m_filterFlags, eLogLevelFilterFlags_Info);
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("warn", &m_filterFlags, eLogLevelFilterFlags_Warn);
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("error", &m_filterFlags, eLogLevelFilterFlags_Error);
            ImGui::SameLine();
            bNeedCollect |= ImGui::CheckboxFlags("critical", &m_filterFlags, eLogLevelFilterFlags_Critical);
        }

        // 렌더링
        const MemorySinkMT::Container& container = log::GetMemorySink()->GetContainer();
        if (container.cbegin() != m_logContainerBeginIt || container.cend() != m_logContainerEndIt)   // 캐싱해둔 iterator와 비교하여 컨테이너가 변경되었는지 확인
        {
            bNeedCollect = true;   // 바뀌었구나!
        }

        if (bNeedCollect)   // 필터링을 포함한 로그 수집
        {
            CollectLogEntries_();          // 수집
            bNeedScroll = m_bAutoScroll;   // 자동 스크롤 예약
        }

        Int32 renderedItemCount = 0;   // 렌더링된 로그 항목 수 초기화 [디버깅용]

        // 렌더링
        ImVec2 tableSize = ImGui::GetContentRegionAvail();      // 테이블 크기
        tableSize.y -= ImGui::GetTextLineHeightWithSpacing();   // 하단에 디버깅 정보를 출력하기 위해서 공간을 남김

        // 테이블 렌더링
        if (ImGui::BeginTable("console_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, tableSize))
        {
            // 테이블 헤더
            {
                ImGui::TableSetupScrollFreeze(0, 1);   // 헤더는 고정
                ImGui::TableSetupColumn("level", ImGuiTableColumnFlags_WidthFixed, 75.f);
                ImGui::TableSetupColumn("log", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
            }

            // 클리핑
            static ImGuiListClipper s_clipper;
            s_clipper.Begin(static_cast<Int32>(m_filteredLogCollection.size()), ImGui::GetTextLineHeightWithSpacing());
            while (s_clipper.Step())
            {
                for (int i = s_clipper.DisplayStart; i < s_clipper.DisplayEnd; i++)
                {
                    const LogItem* pLog     = m_filteredLogCollection[i];
                    const ImVec4&   logColor = kLogColors[EnumToInt(pLog->level)];

                    ImGui::TableNextRow();

                    // 로그 레벨 렌더링
                    ImGui::TableNextColumn();
                    ImGui::TextColored(logColor, EnumToString(pLog->level).data());

                    // 로그 메시지 렌더링
                    ImGui::TableNextColumn();
                    ImGui::TextColored(logColor, pLog->log.c_str());

                    renderedItemCount++;   // 렌더링된 아이템 카운트 증가
                }
            }

            // 자동 스크롤
            if (bNeedScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);   // 테이블의 맨 아래로 스크롤
            }

            ImGui::EndTable();
        }

        // 디버깅 정보 렌더링
        ImGui::TextDisabled("count: %zu, capacity: %zu, filtered: %zu, rendered: %d", container.size(), container.capacity(), m_filteredLogCollection.size(), renderedItemCount);
    }
    ImGui::End();
}

void ConsolePanel::CollectLogEntries_()
{
    const Ref<MemorySinkMT>&       memorySink = log::GetMemorySink();
    const MemorySinkMT::Container& container  = memorySink->GetContainer();

    // 필터링 수행
    m_filteredLogCollection.clear();
    for (const LogItem& entry: container)
    {
        // 로그 레벨 필터링
        if ((m_filterFlags & (1 << entry.level)) == 0)
        {
            continue;
        }

        // 문자열 필터링
        if (m_textFilter.IsActive() && (m_textFilter.PassFilter(entry.log.c_str()) == false))
        {
            continue;
        }

        // 모든 필터 통과
        m_filteredLogCollection.push_back(&entry);
    }

    m_logContainerBeginIt = container.cbegin();   // 현재 컨테이너의 begin을 저장하여 다음에 변경되었는지 확인할 수 있도록 함
    m_logContainerEndIt   = container.cend();     // 현재 컨테이너의 end를 저장하여 다음에 변경되었는지 확인할 수 있도록 함
}

}   // namespace jam