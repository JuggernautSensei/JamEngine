#include "pch.h"

#include "Editor_Profiler.h"

#include "Application.h"
#include "Input.h"

namespace jam
{

void Editor_Profiler::OnImGuiRender(bool& _pShow) const
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

void Editor_Profiler::DrawFrameTabItem_() const
{
    const Application& app         = Application::GetInstance();
    float              deltaTimeMs = app.GetDeltaTimeSec() / 0.001f;
    float              frameRate   = app.GetFrameRate();

    ImGui::Text("delta time: %.3f ms", deltaTimeMs);
    ImGui::Text("frame rate: %.1f fps", frameRate);
}

void Editor_Profiler::DrawInputTabItem_() const
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

    constexpr const char* k_mouseName[EnumCount<eMouseButton>()] = {
        "left",
        "right",
        "middle",
        "button 4",
        "button 5",
    };

    constexpr eMouse k_mouseButtonTable[EnumCount<eMouseButton>()] = {
        eMouse::LeftButton,
        eMouse::RightButton,
        eMouse::MiddleButton,
        eMouse::XButton1,
        eMouse::XButton2,
    };

    enum class eMouseState
    {
        Pressed,
        Hold,
        Released,
        Away
    };

    constexpr const char* k_mouseStateName[EnumCount<eMouseState>()] = {
        "pressed",
        "hold",
        "released",
        "away"
    };

    const auto [x, y]      = Input::GetMousePos();
    const auto [dx, dy]    = Input::GetMouseDeltaPos();
    const float wheelDelta = Input::GetMouseDeltaWheel();

    const bool bVisible = Input::IsMouseVisible();
    const bool bGrabbed = Input::IsMouseCursorGrabbed();

    ImGui::BulletText("game mouse");
    ImGui::TreePush("game mouse");
    {
        ImGui::Text("mouse position: (%d, %d)", x, y);
        ImGui::Text("mouse delta position: (%d, %d)", dx, dy);
        ImGui::Text("mouse delta wheel: %.1f", wheelDelta);
        for (const eMouseButton e: EnumRange<eMouseButton>())
        {
            const Int32  idx    = EnumToInt(e);
            const eMouse button = k_mouseButtonTable[idx];
            eMouseState  state  = eMouseState::Away;

            if (Input::IsMousePressed(button))
                state = eMouseState::Pressed;
            else if (Input::IsMouseHold(button))
                state = eMouseState::Hold;
            else if (Input::IsMouseReleased(button))
                state = eMouseState::Released;

            ImGui::Text("mouse %s: %s", k_mouseName[idx], k_mouseStateName[EnumToInt(state)]);
        }

        ImGui::Text("mouse visible: %s", bVisible ? "true" : "false");
        ImGui::Text("mouse cursor grabbed: %s", bGrabbed ? "true" : "false");
    }
    ImGui::TreePop();

    ImGui::BulletText("application input");
    ImGui::TreePush("application mouse");
    {
        if (ImGui::IsMousePosValid())
            ImGui::Text("mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("mouse pos: <INVALID>");
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
        ImGuiKey start_key = ImGuiKey_NamedKey_BEGIN;
        ImGui::Text("Keys down:");
        for (ImGuiKey key = start_key; key < ImGuiKey_NamedKey_END; key = static_cast<ImGuiKey>(key + 1))
        {
            if (Func::IsLegacyNativeDupe(key) || !ImGui::IsKeyDown(key))
                continue;

            ImGui::SameLine();
            ImGui::Text((key < ImGuiKey_NamedKey_BEGIN) ? "\"%s\"" : "\"%s\" %d", ImGui::GetKeyName(key), key);
        }
        ImGui::Text("Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
        ImGui::Text("Chars queue:");
        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            ImWchar c = io.InputQueueCharacters[i];
            ImGui::SameLine();
            ImGui::Text("\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? static_cast<char>(c) : '?', c);
        }
    }
    ImGui::TreePop();
}

}   // namespace jam
