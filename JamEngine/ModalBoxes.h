#pragma once
#include "EditorUtilities.h"
#include "IModalBox.h"

namespace jam
{

class OkModalBox : public IModalBoxPanel
{
public:
    OkModalBox(const std::string_view _title, const std::string_view _msg)
        : m_title(_title)
        , m_msg(_msg)
    {
    }

    void Show() override
    {
        // log
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
        ImGui::TextUnformatted(m_msg.c_str());
        ImGui::PopTextWrapPos();

        // ok 버튼
        if (ImGui::Button("ok", editor::CalcFittedButtonSize(1)))
        {
            ImGui::CloseCurrentPopup();
        }
    }

    JAM_NODISCARD std::string_view GetTitle() const override
    {
        return m_title;
    }

private:
    std::string m_title;
    std::string m_msg;
};

// custom log box with a callback function
// fast way to create a log box with lambda
class CallbackModalBox : public IModalBoxPanel
{
public:
    CallbackModalBox(const std::string_view _title, const std::function<void()>& _showCallback)
        : m_title(_title)
        , m_showCallback(_showCallback)
    {
        JAM_ASSERT(_showCallback, "Show callback function must not be null.");
    }

    void Show() override
    {
        m_showCallback();
    }

    JAM_NODISCARD std::string_view GetTitle() const override
    {
        return m_title;
    }

private:
    std::string           m_title;          // 메시지 박스 제목
    std::function<void()> m_showCallback;   // 콜백 함수
};

// yes/no log box with two callback functions
class YesOrNoModalBox : public IModalBoxPanel
{
public:
    explicit YesOrNoModalBox(const std::string_view _title, const std::string_view _msg, const std::function<void()>& _onPressYesCallbackOrNull = nullptr, const std::function<void()>& _onPressNoCallbackOrNull = nullptr)
        : m_title(_title)
        , m_msg(_msg)
        , m_onPressYesCallbackOrNull(_onPressYesCallbackOrNull)
        , m_onPressNoCallbackOrNull(_onPressNoCallbackOrNull)
    {
    }

    void Show() override
    {
        // 메시지 박스 제목과 내용
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
        ImGui::TextUnformatted(m_title.c_str());
        ImGui::TextUnformatted(m_msg.c_str());
        ImGui::PopTextWrapPos();

        // 버튼의 너비 계산
        ImVec2 buttonSize = editor::CalcFittedButtonSize(2);

        // yes 버튼
        if (ImGui::Button("yes", buttonSize))
        {
            if (m_onPressYesCallbackOrNull)
            {
                m_onPressYesCallbackOrNull();
            }
            ImGui::CloseCurrentPopup();
        }

        // no 버튼
        ImGui::SameLine();
        if (ImGui::Button("no", buttonSize))
        {
            if (m_onPressNoCallbackOrNull)
            {
                m_onPressNoCallbackOrNull();
            }
            ImGui::CloseCurrentPopup();
        }
    }

    JAM_NODISCARD std::string_view GetTitle() const override
    {
        return m_title;
    }

private:
    std::string           m_title;                      // 메시지 박스 제목
    std::string           m_msg;                        // 메시지 박스 내용
    std::function<void()> m_onPressYesCallbackOrNull;   // 예 버튼 콜백 함수
    std::function<void()> m_onPressNoCallbackOrNull;    // 아니오 버튼 콜백 함수
};
}   // namespace jam