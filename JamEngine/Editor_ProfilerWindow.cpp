#include "pch.h"

#include "Editor_ProfilerWindow.h"

#include "Application.h"
#include "ImGuiGlue.h"
#include "Input.h"

namespace Jam
{

void Editor_ProfilerWindow::OnImGuiRender(bool& _pShow) const
{
    if (!ImGui::Begin("profiler", &_pShow))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("profileMenu"))
    {
        if (ImGui::BeginTabItem("frame"))
        {
            DrawFrameTabItem_();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("input"))
        {
            DrawInputTabItem_();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void Editor_ProfilerWindow::DrawFrameTabItem_() const
{

    const float deltaTimeMs = JAM_APP.GetDeltaTimeSec() / 0.001f;
    const float frameRate   = JAM_APP.GetFrameRate();

    ImGui::Text("delta time: %.3f ms", deltaTimeMs);
    ImGui::Text("frame rate: %.1f fps", frameRate);
}

void Editor_ProfilerWindow::DrawInputTabItem_() const
{
    ImGuiIO& io = ImGui::GetIO();

    // mouse
    enum class eMouseButton
    {
        Left = 0,
        Right,
        Middle,
        Button4,
        Button5,
    };

    constexpr EnumArray<eMouseButton, eMouseButton> k_mouseButtonRanges = {
        eMouseButton::Left,
        eMouseButton::Right,
        eMouseButton::Middle,
        eMouseButton::Button4,
        eMouseButton::Button5
    };

    enum class eMouseState
    {
        Pressed = 0,
        Hold,
        Released,
        Away,

    };

    constexpr EnumArray<eMouseButton, const char*> k_mouseNames = {
        "left",
        "right",
        "middle",
        "button 4",
        "button 5",
    };

    constexpr EnumArray<eMouseButton, eMouse> k_mouseButtons = {
        eMouse::LeftButton,
        eMouse::RightButton,
        eMouse::MiddleButton,
        eMouse::XButton1,
        eMouse::XButton2,
    };

    constexpr EnumArray<eMouseState, const char*> k_mouseStateNames = {
        "pressed",
        "hold",
        "released",
        "away"
    };

    const auto [x, y]      = JAM_INPUT.GetMousePos();
    const auto [dx, dy]    = JAM_INPUT.GetMouseDeltaPos();
    const float wheelDelta = JAM_INPUT.GetMouseDeltaWheel();
    const bool  bVisible   = JAM_INPUT.IsMouseVisible();

    ImGui::BulletText("game mouse");
    ImGui::TreePush("game mouse");
    {
        ImGui::Text("mouse position: (%d, %d)", x, y);
        ImGui::Text("mouse delta position: (%d, %d)", dx, dy);
        ImGui::Text("mouse delta wheel: %.1f", wheelDelta);
        for (const eMouseButton e: k_mouseButtonRanges)
        {
            const Int32  idx    = UnderlyingFrom(e);
            const eMouse button = k_mouseButtons[idx];
            eMouseState  state  = eMouseState::Away;

            if (JAM_INPUT.IsMousePressed(button))
            {
                state = eMouseState::Pressed;
            }
            else if (JAM_INPUT.IsMouseHold(button))
            {
                state = eMouseState::Hold;
            }
            else if (JAM_INPUT.IsMouseReleased(button))
            {
                state = eMouseState::Released;
            }

            ImGui::Text("mouse %s: %s", k_mouseNames[idx], k_mouseStateNames[UnderlyingFrom(state)]);
        }

        ImGui::Text("mouse visible: %s", bVisible ? "true" : "false");
    }
    ImGui::TreePop();

    ImGui::BulletText("application input");
    ImGui::TreePush("application mouse");
    {
        if (ImGui::IsMousePosValid())
        {
            ImGui::Text("mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
        }
        else
        {
            ImGui::Text("mouse pos: <INVALID>");
        }

        ImGui::Text("mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);
        ImGui::Text("mouse down:");
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
        {
            if (ImGui::IsMouseDown(i))
            {
                ImGui::SameLine();
                ImGui::Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]);
            }
        }
        ImGui::Text("mouse wheel: %.1f", io.MouseWheel);
        ImGui::Text("mouse clicked count:");
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
        {
            if (io.MouseClickedCount[i] > 0)
            {
                ImGui::SameLine();
                ImGui::Text("b%d: %d", i, io.MouseClickedCount[i]);
            }
        }
    }
    ImGui::TreePop();

    ImGui::TreePush("application keyboard");
    {
        struct Func
        {
            static bool IsLegacyNativeDupe(ImGuiKey) { return false; }
        };
        constexpr ImGuiKey start_key = ImGuiKey_NamedKey_BEGIN;
        ImGui::Text("Keys down:");
        for (ImGuiKey key = start_key; key < ImGuiKey_NamedKey_END; key = static_cast<ImGuiKey>(key + 1))
        {
            if (Func::IsLegacyNativeDupe(key) || !ImGui::IsKeyDown(key))
            {
                continue;
            }

            ImGui::SameLine();
            ImGui::Text((key < ImGuiKey_NamedKey_BEGIN) ? "\"%s\"" : "\"%s\" %d", ImGui::GetKeyName(key), key);
        }
        ImGui::Text("Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
        ImGui::Text("Chars queue:");
        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            const ImWchar c = io.InputQueueCharacters[i];
            ImGui::SameLine();
            ImGui::Text("\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? static_cast<char>(c) : '?', c);
        }
    }
    ImGui::TreePop();
}

}   // namespace Jam
