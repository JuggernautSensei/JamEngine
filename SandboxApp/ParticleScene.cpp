#include "pch.h"
#include "ParticleScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"

#define SCENE_PATH(_filepath) "Resources/ParticleScene/" _filepath

namespace
{
struct CBSimulation
{
    float deltaTime, time, wind, size;
    Int32 count, previousCount, effect, seed;
    Vec3  extent;
    float fallSpeed;
};

struct CBCamera
{
    Mtx4  viewProj, invViewProj;
    Vec3  right;
    float padding0;
    Vec3  up;
    float padding1;
    Vec3  eye;
    float padding2;
};
static_assert(sizeof(CBSimulation) == 48 && sizeof(CBCamera) == 176);
}   // namespace

ParticleScene::ParticleScene()
{
    m_camera.transform.SetTranslation({ 0.f, 4.5f, -18.f });
    m_camera.camera.SetFarZ(100.f);
    m_camera.dMove = 6.f;
    InitRenderGraph_();
}

void ParticleScene::RunDemo(const Int32 _index)
{
    m_effect    = Clamp(_index, 0, 1);
    m_count     = m_effect == 0 ? 12000 : 6000;
    m_wind      = 1.f;
    m_size      = 1.f;
    m_fallSpeed = 1.f;
    Restart_();
}

void ParticleScene::SetParticleCount(const Int32 _count)
{
    m_count = Clamp(_count, 0, kCapacity);
}

void ParticleScene::OnUpdate(const float _deltaTime)
{
    m_camera.Update(_deltaTime);
    m_deltaTime = Clamp(_deltaTime, 0.f, 1.f / 15.f);
    if (!m_bPaused)
        m_time += m_deltaTime;
}

void ParticleScene::OnRender(const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBSimulation simulation  = {};
    simulation.deltaTime     = m_bPaused ? 0.f : m_deltaTime;
    simulation.time          = m_time;
    simulation.wind          = m_wind;
    simulation.size          = m_size;
    simulation.count         = m_count;
    simulation.previousCount = m_bReset ? 0 : m_previousCount;
    simulation.effect        = m_effect;
    simulation.seed          = m_seed;
    simulation.extent        = { 12.f, 10.f, 8.f };
    simulation.fallSpeed     = m_fallSpeed;
    gfx.UpdateBuffer(m_simulationCbh, AsMemoryView(simulation));

    CBCamera camera    = {};
    camera.viewProj    = m_camera.CalcViewProj().Transpose();
    camera.invViewProj = m_camera.CalcInvViewProj().Transpose();
    camera.right       = m_camera.transform.GetRight();
    camera.up          = m_camera.transform.GetUp();
    camera.eye         = m_camera.GetEyePos();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(camera));

    if (m_count != m_lastDrawCount)
    {
        const Int32 args[8] = { 6, m_count, 0, 0, 0, 0, 0, 0 };
        static_assert(sizeof(args) == kDrawIndirectStride);
        gfx.UpdateBuffer(m_drawSbh, AsMemoryView(args), 0, false);
        m_lastDrawCount = m_count;
    }
    m_graph.Execute(_view.fbh, _view.viewport);
    m_previousCount = m_count;
    m_bReset        = false;
}

void ParticleScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("ParticleScene"))
    {
        ImGui::End();
        return;
    }

    Int32 effect = m_effect;
    if (ImGui::Combo("Effect", &effect, "Snow\0Confetti\0"))
        RunDemo(effect);
    ImGui::SliderInt("Particles", &m_count, 0, kCapacity);
    m_count = Clamp(m_count, 0, kCapacity);
    ImGui::Checkbox("Pause", &m_bPaused);
    ImGui::SameLine();
    if (ImGui::Button("Restart"))
        Restart_();
    ImGui::SliderFloat("Wind", &m_wind, -4.f, 4.f);
    ImGui::SliderFloat("Size", &m_size, 0.25f, 3.f);
    ImGui::SliderFloat("Fall speed", &m_fallSpeed, 0.25f, 3.f);
    ImGui::TextUnformatted("WASD / QE: move, RMB drag: look");
    ImGui::End();
}

void ParticleScene::OnResize(const Int32 _width, const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void ParticleScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    AttachmentRG attachments[2] = {};
    attachments[0].name         = "T_SceneColor";
    attachments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attachments);

    m_graph.EmplaceStructuredBuffer("B_Particles", kCapacity, sizeof(Particle), eStructureBuffer::Default, eStorageBufferFlags::ShaderRead | eStorageBufferFlags::ShaderReadWrite);
    m_graph.EmplaceIndirectBuffer("B_Draw", 1);
    m_drawSbh = m_graph.GetStorageBuffer("B_Draw");
    m_graph.EmplaceConstantBuffer("CB_Simulation", sizeof(CBSimulation));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_simulationCbh = m_graph.GetConstantBuffer("CB_Simulation");
    m_cameraCbh     = m_graph.GetConstantBuffer("CB_Camera");

    LoadProgram(m_graph, "P_Simulate", SCENE_PATH("SimulateShader.hlsl"), eShaderFlags::Compute);
    LoadProgram(m_graph, "P_Background", SCENE_PATH("BackgroundShader.hlsl"));
    LoadProgram(m_graph, "P_Particles", SCENE_PATH("ParticleShader.hlsl"));

    m_graph.AddComputePass("Pass_Simulate")
        .SetProgram("P_Simulate")
        .SetConstantBuffer("CB_Simulation", 0)
        .SetBuffer("B_Particles", 0, eResourceAccess::ReadWrite)
        .SetExecutor(Bind<&ParticleScene::Simulate_>(this));

    m_graph.AddRenderPass("Pass_Background")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Background")
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetConstantBuffer("CB_Simulation", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetExecutor(Bind<&ParticleScene::SubmitBackground_>(this));

    m_graph.AddRenderPass("Pass_Particles")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Particles")
        .SetRenderFlags(eRenderFlags::DepthFunc_Less)
        .SetBlendFlags(kAlphaBlend, 0)
        .SetConstantBuffer("CB_Simulation", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Simulation", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetBuffer("B_Particles", eShader::VertexShader, 0)
        .SetExecutor(Bind<&ParticleScene::SubmitParticles_>(this));

    ImageFilterInfo blit = {};
    blit.filter          = eImageFilter::Blit;
    blit.output          = eImageFilterOutput::Final;
    blit.input0Texture   = "T_SceneColor";
    GetImageFilterManager().AddImageFilter(m_graph, "Pass_Blit", blit);
}

void ParticleScene::Simulate_() const
{
    if (m_count > 0)
        GetGraphics().Dispatch((m_count + 255) / 256, 1, 1);
}

void ParticleScene::SubmitBackground_() const
{
    GetImageFilterManager().SubmitScreenTriangle();
}

void ParticleScene::SubmitParticles_() const
{
    if (m_count == 0) return;
    Graphics& gfx = GetGraphics();
    if (m_effect == 1)
    {
        gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite);
        gfx.SetBlendFlags(kOpaqueBlend, 0);
    }
    gfx.SetVertexBuffer(kInvalidHandle);
    gfx.SetIndexBuffer(kInvalidHandle);
    gfx.Submit(m_drawSbh);
}

void ParticleScene::Restart_()
{
    m_seed   = m_seed % 65535 + 1;
    m_time   = 0.f;
    m_bReset = true;
}
