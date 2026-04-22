#include "pch.h"

#include "ToolbarPanel.h"

#include "EditorLayer.h"
#include "EditorUtilities.h"

namespace jam
{

ToolbarPanel::ToolbarPanel(EditorLayer* _pEditorLayer)
    : m_pEditorLayer(_pEditorLayer)
{
    JAM_ASSERT(_pEditorLayer, "Editor layer pointer is null.");
}

void ToolbarPanel::Show(bool* _pOpen) const
{
    if (ImGui::Begin("toolbar", _pOpen))
    {
        // 버튼 크기 계산
        ImVec2 buttonSize = ImGui::GetContentRegionAvail();
        buttonSize.x      = buttonSize.y;   // 정사각형 버튼

        eRunMode mode = m_pEditorLayer->GetRunMode();
        switch (mode)
        {
            case eRunMode::Edit:
            {
                // ▶ ▶|
                DrawPlayButton_(buttonSize);
                ImGui::SameLine();
                DrawSimulateButton_(buttonSize);
            }
            break;

            case eRunMode::Play:
            {
                // ■
                DrawResetButton_(buttonSize);
            }
            break;

            case eRunMode::Simulate:
            {
                // ∥ ▶| ■
                DrawStepButton(buttonSize);
                ImGui::SameLine();
                DrawPauseButton_(buttonSize);
                ImGui::SameLine();
                DrawResetButton_(buttonSize);
            }
            break;
        }
    }
    ImGui::End();
}

void ToolbarPanel::DrawPlayButton_(const ImVec2& _imageSize) const
{
    if (editor::ImageButton("##playButton", m_playIcon.GetSRV(), _imageSize))
    {
        Application::GetInstance().SetUpdatePolicy(eUpdatePolicy::PerFrame);
        m_pEditorLayer->SetRunMode(eRunMode::Play);
    }
}

void ToolbarPanel::DrawPauseButton_(const ImVec2& _imageSize) const
{
    if (editor::ImageButton("##stopButton", m_pauseICon.GetSRV(), _imageSize))
    {
        JAM_ASSERT(m_pEditorLayer->GetRunMode() == eRunMode::Simulate, "Run mode must be simulate mode to pause.");
        Application::GetInstance().SetUpdatePolicy(eUpdatePolicy::FixedStep);
        m_pEditorLayer->SetRunMode(eRunMode::Simulate);
    }
}

void ToolbarPanel::DrawSimulateButton_(const ImVec2& _imageSize) const
{
    if (editor::ImageButton("##simulateButton", m_simulateIcon.GetSRV(), _imageSize))
    {
        Application::GetInstance().SetUpdatePolicy(eUpdatePolicy::PerFrame);
        m_pEditorLayer->SetRunMode(eRunMode::Simulate);
    }
}

void ToolbarPanel::DrawResetButton_(const ImVec2& _imageSize) const
{
    if (editor::ImageButton("##resetButton", m_resetIcon.GetSRV(), _imageSize))
    {
        // todo. 씬을 초기화하는
        Application::GetInstance().SetUpdatePolicy(eUpdatePolicy::FixedStep);
        m_pEditorLayer->SetRunMode(eRunMode::Edit);
    }
}

void ToolbarPanel::DrawStepButton(const ImVec2& _imageSize) const
{
    if (editor::ImageButton("##stepButton", m_simulateIcon.GetSRV(), _imageSize))
    {
        Application& app = Application::GetInstance();
        app.SetUpdatePolicy(eUpdatePolicy::FixedStep);
        app.Step();

        m_pEditorLayer->SetRunMode(eRunMode::Simulate);
    }
}

}   // namespace jam