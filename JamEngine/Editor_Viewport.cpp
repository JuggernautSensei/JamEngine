#include "pch.h"

#include "Editor_Viewport.h"

#include "Graphics.h"
#include "SceneManager.h"

namespace jam
{

void Editor_Viewport::Init()
{
    constexpr UInt32 k_initWidth  = 1280;
    constexpr UInt32 k_initHeight = 720;
    Resize_(k_initWidth, k_initHeight);
}

void Editor_Viewport::Shutdown()
{
    JAM_DESTROY_TEXTURE(m_viewportTh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
}

void Editor_Viewport::Update(const float _deltaSec)
{
    switch (m_mode)
    {
        case eMode::Edit:
            UpdateEdit_(_deltaSec);
            break;
        case eMode::Run:
            UpdateRun_(_deltaSec);
            break;
        case eMode::Simulate:
            UpdateSimulate_(_deltaSec);
            break;
        default:
            JAM_ASSERT(0, "Unknown editor viewport mode: {}", static_cast<UInt8>(m_mode));
    }
}

void Editor_Viewport::Render() const
{
    RenderView view;
    view.eye  = m_cameraTransform.GetTranslation();
    view.view = m_cameraTransform.GetViewMatrix();
    view.proj = m_camera.CreateProj();

    view.frameBufferHandle = m_fbh;
    view.viewport          = m_viewport;

    SceneManager::RenderOffScreen(view);
}

void Editor_Viewport::OnImGuiRender()
{
    if (!ImGui::Begin("viewport"))
    {
        ImGui::End();
        return;
    }

    // viewport menu
    const char* modeName = EnumToString(m_mode).data();
    ImGui::Text("mode: %s", modeName);
    if (m_mode == eMode::Edit)
    {
        DrawSimulateButton();
        ImGui::SameLine();
        DrawPlayButton();
    }
    else if (m_mode == eMode::Simulate)
    {
        if (m_simulateMode == eSimulateMode::Pause)
        {
            DrawSimulatePlayButton();
            ImGui::SameLine();
            DrawStepButton();
        }
        else if (m_simulateMode == eSimulateMode::Play)
        {
            DrawSimulatePauseButton();
        }
        ImGui::SameLine();
        DrawStopButton();

        if (m_simulateMode == eSimulateMode::Pause)
        {
            ImGui::SameLine();
            float stepDtMs = m_stepDt * 1000.f;
            if (ImGui::DragFloat("step delta", &stepDtMs, 0.1f, 0.0f, 1000.f, "%.3f ms"))
                m_stepDt = stepDtMs / 1000.f;
        }
    }
    else
    {
        DrawStopButton();
    }

    // viewport image
    ImVec2 regionRes = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("viewport image", regionRes))
    {
        auto [w, h] = ImGui::GetContentRegionAvail();

        // if size changed, resize viewport
        if (m_viewport.w != static_cast<UInt16>(w) || m_viewport.h != static_cast<UInt16>(h))
            Resize_(static_cast<Int32>(w), static_cast<Int32>(h));
    }
    ImGui::EndChild();

    ImGui::End();
}

void Editor_Viewport::Resize_(const Int32 _width, const Int32 _height)
{
    JAM_DESTROY_TEXTURE(m_viewportTh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    m_viewport.w = NarrowCast<UInt16>(_width);
    m_viewport.h = NarrowCast<UInt16>(_height);

    m_viewportTh = Graphics::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
    m_fbh        = Graphics::CreateFrameBuffer(m_viewportTh);

    m_camera.SetWidth(static_cast<float>(_width));
    m_camera.SetHeight(static_cast<float>(_height));

    // scene manager resize
    SceneManager::Resize(_width, _height);
}

void Editor_Viewport::UpdateEdit_(float _deltaSec) const
{
    // In edit mode, we might want to update only certain aspects of the scene.
}

void Editor_Viewport::UpdateRun_(const float _deltaSec) const
{
    SceneManager::Update(_deltaSec);
}

void Editor_Viewport::UpdateSimulate_(const float _deltaSec)
{
    if (m_simulateMode == eSimulateMode::Play)
    {
        SceneManager::Update(_deltaSec);
    }
    else if (m_simulateMode == eSimulateMode::Step)
    {
        SceneManager::Update(m_stepDt);
        m_simulateMode = eSimulateMode::Pause;
    }
}

void Editor_Viewport::DrawSimulateButton()
{
    if (ImGui::Button("simulate"))
    {
        m_mode         = eMode::Simulate;
        m_simulateMode = eSimulateMode::Play;
    }
}

void Editor_Viewport::DrawSimulatePauseButton()
{
    if (ImGui::Button("pause"))
    {
        m_mode         = eMode::Simulate;
        m_simulateMode = eSimulateMode::Pause;
    }
}

void Editor_Viewport::DrawSimulatePlayButton()
{
    if (ImGui::Button("play"))
    {
        m_mode         = eMode::Simulate;
        m_simulateMode = eSimulateMode::Play;
    }
}

void Editor_Viewport::DrawPlayButton()
{
    if (ImGui::Button("play"))
    {
        m_simulateMode = eSimulateMode::Play;
    }
}

void Editor_Viewport::DrawStopButton()
{
    if (ImGui::Button("stop"))
    {
        m_mode = eMode::Edit;
    }
}

void Editor_Viewport::DrawStepButton()
{
    if (ImGui::Button("step"))
    {
        m_simulateMode = eSimulateMode::Step;
    }
}

}   // namespace jam