#include "pch.h"

#include "MainMenuBarPanel.h"

#include "Application.h"
#include "EditorLayer.h"

namespace jam
{

MainMenuBarPanel::MainMenuBarPanel(EditorLayer* _pEditorLayer)
    : m_pEditorLayer(_pEditorLayer)
{
    JAM_ASSERT(_pEditorLayer, "EditorLayer pointer must not be null.");
}

void MainMenuBarPanel::Show() const
{
    if (ImGui::BeginMainMenuBar())
    {
#pragma region File Menu Item
        if (ImGui::BeginMenu("file"))
        {
            if (ImGui::MenuItem("exit"))
            {
                Application::GetInstance().Quit();
            }
            ImGui::EndMenu();
        }
#pragma endregion

#pragma region Window
        if (ImGui::BeginMenu("window"))
        {
            if (ImGui::MenuItem("debug panel", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::Debug)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::Debug);
            }

            if (ImGui::MenuItem("viewport", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::Viewport)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::Viewport);
            }

            if (ImGui::MenuItem("scene hierarchy", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::SceneHierarchy)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::SceneHierarchy);
            }

            if (ImGui::MenuItem("asset inspector", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::AssetInspector)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::AssetInspector);
            }

            if (ImGui::MenuItem("entity inspector", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::EntityInspector)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::EntityInspector);
            }

            if (ImGui::MenuItem("contents brower", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::ContentsBrowser)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::ContentsBrowser);
            }

            if (ImGui::MenuItem("console", nullptr, m_pEditorLayer->IsPanelVisible(ePanel::Console)))
            {
                m_pEditorLayer->TogglePanelVisible(ePanel::Console);
            }

            ImGui::EndMenu();
        }
#pragma endregion

#pragma region theme
        // 테마 변경 메뉴
        if (ImGui::BeginMenu("theme"))
        {
            for (eEditorTheme theme: EnumRange<eEditorTheme>())
            {
                if (ImGui::MenuItem(EnumToString(theme).data(), nullptr, m_pEditorLayer->GetEditorTheme() == theme))
                {
                    m_pEditorLayer->SetEditorTheme(theme);
                }
            }
            ImGui::EndMenu();
        }
#pragma endregion

        ImGui::EndMainMenuBar();
    }
}

}   // namespace jam