#include "pch.h"

#include "DebugPanel.h"

#include "Application.h"
#include "Input.h"
#include "Timer.h"

namespace jam
{

void DebugPanel::Show(bool* _pOpenOrNul) const
{
    if (ImGui::Begin("debug panel", _pOpenOrNul))
    {
        if (ImGui::BeginTabBar("debug tab bar"))
        {
#pragma region Frame Performence Panel
            if (ImGui::BeginTabItem("frame performence"))
            {
                const TickTimer& timer    = Application::GetInstance().GetTimer();
                float            deltaSec = timer.GetDeltaSec();
                ImGui::Text("frame rate: %.4f hz", 1.f / deltaSec);
                ImGui::Text("delta milli sec: %.4f ms", deltaSec * 1e-3);
                ImGui::EndTabItem();
            }
#pragma endregion

#pragma region Input Test
            if (ImGui::BeginTabItem("input test"))
            {
                auto [mx, my] = Input::GetMousePos();
                ImGui::Text("mouse position: (%u, %u)", mx, my);

                auto [dx, dy] = Input::GetMouseDeltaPos();
                ImGui::Text("mouse delta position: (%u, %u)", dx, dy);

                Int32 mouseWheelDelta = Input::GetMouseDeltaWheel();
                ImGui::Text("mouse wheel delta: %d", mouseWheelDelta);

                if (ImGui::TreeNode("mouse actions"))
                {
                    if (ImGui::BeginChild("mouse actions view", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border))
                    {
                        // mouse button state
                        for (eMouse mouse: EnumRange<eMouse>())
                        {
                            if (mouse == eMouse::None)
                                continue;

                            bool isDown     = Input::IsMouseDown(mouse);
                            bool isPressed  = Input::IsMousePressed(mouse);
                            bool isHold     = Input::IsMouseHold(mouse);
                            bool isUp       = Input::IsMouseUp(mouse);
                            bool isReleased = Input::IsMouseReleased(mouse);
                            bool isAway     = Input::IsMouseAway(mouse);

                            ImGui::Text("Mouse %s: Down=%s, Pressed=%s, Hold=%s, Up=%s, Released=%s, Away=%s",
                                        magic_enum::enum_name(mouse).data(),
                                        isDown ? "true" : "false",
                                        isPressed ? "true" : "false",
                                        isHold ? "true" : "false",
                                        isUp ? "true" : "false",
                                        isReleased ? "true" : "false",
                                        isAway ? "true" : "false");
                        }
                    }
                    ImGui::EndChild();
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("keyboard actions"))
                {
                    if (ImGui::BeginChild("keyboard actions view", ImVec2 { 0.f, 300.f }, ImGuiChildFlags_Border))
                    {
                        // keyboard key state
                        for (eKey key: EnumRange<eKey>())
                        {
                            if (key == eKey::None)
                                continue;

                            bool isDown     = Input::IsKeyDown(key);
                            bool isPressed  = Input::IsKeyPressed(key);
                            bool isHold     = Input::IsKeyHold(key);
                            bool isUp       = Input::IsKeyUp(key);
                            bool isReleased = Input::IsKeyReleased(key);
                            bool isAway     = Input::IsKeyAway(key);
                            ImGui::Text("Key %s: Down=%s, Pressed=%s, Hold=%s, Up=%s, Released=%s, Away=%s",
                                        magic_enum::enum_name(key).data(),
                                        isDown ? "true" : "false",
                                        isPressed ? "true" : "false",
                                        isHold ? "true" : "false",
                                        isUp ? "true" : "false",
                                        isReleased ? "true" : "false",
                                        isAway ? "true" : "false");
                        }
                    }
                    ImGui::EndChild();
                    ImGui::TreePop();
                }

                ImGui::EndTabItem();
            }
#pragma endregion

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

}   // namespace jam