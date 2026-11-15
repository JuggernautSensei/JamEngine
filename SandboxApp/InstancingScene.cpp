#include "pch.h"
#include "InstancingScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"

#define SCENE_PATH(_filepath) "Resources/InstancingScene/" _filepath

namespace
{

struct CBNoInstancing
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world;
};

struct CBInstancing
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding1;
    Mtx4  viewProj;
};

}   // namespace

void InstancingScene::InitMeshes_()
{
    Graphics& gfx = GetGraphics();

    struct Vertex
    {
        Vec3 point;
        RGBA rgba;
    };

    VertexLayout layout = {};
    layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
    layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

    Vertex verts[] = {
        { { -0.5f, -0.5f, -0.5f }, RGBA::FromLinear(1.0f, 0.0f, 0.0f, 1.0f) },
        {  { 0.5f, -0.5f, -0.5f }, RGBA::FromLinear(0.0f, 1.0f, 0.0f, 1.0f) },
        {   { 0.5f, 0.5f, -0.5f }, RGBA::FromLinear(0.0f, 0.0f, 1.0f, 1.0f) },
        {  { -0.5f, 0.5f, -0.5f }, RGBA::FromLinear(1.0f, 1.0f, 0.0f, 1.0f) },
        {  { -0.5f, -0.5f, 0.5f }, RGBA::FromLinear(1.0f, 0.0f, 1.0f, 1.0f) },
        {   { 0.5f, -0.5f, 0.5f }, RGBA::FromLinear(0.0f, 1.0f, 1.0f, 1.0f) },
        {    { 0.5f, 0.5f, 0.5f }, RGBA::FromLinear(1.0f, 0.5f, 0.0f, 1.0f) },
        {   { -0.5f, 0.5f, 0.5f }, RGBA::FromLinear(1.0f, 0.0f, 0.5f, 1.0f) }
    };

    UInt16 idxes[] = { 4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 0, 4, 7, 0, 7, 3, 5, 1, 2, 5, 2, 6, 1, 5, 4, 1, 4, 0, 7, 6, 2, 7, 2, 3 };

    m_vbh = gfx.CreateVertexBuffer(verts, layout);
    m_ibh = gfx.CreateIndexBuffer(idxes);
}

InstancingScene::InstancingScene()
{
    InitMeshes_();
    InitRenderGraph_();

    m_camera.transform.SetTranslationZ(-25.f);
}

InstancingScene::~InstancingScene()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_VERTEX_BUFFER(m_instanceVbh);
}

void InstancingScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_rot += m_dRot * _deltaTime;
    }
}

void InstancingScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBInstancing cbCamera = {};
    cbCamera.eyePos       = m_camera.GetEyePos();
    cbCamera.viewProj     = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void InstancingScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("InstancingScene"))
    {
        ImGui::End();
        return;
    }

    // Instancing
    ImGui::SeparatorText("Rendering");
    ImGui::Checkbox("Instancing", &m_bDoInstancing);
    ImGui::SliderInt("Entities Per Row", &m_numEntitiesPerRow, 1, 256);
    ImGui::Text("Total Entities: %d", m_numEntitiesPerRow * m_numEntitiesPerRow);

    // Animation
    ImGui::SeparatorText("Animation");
    ImGui::Checkbox("Animate", &m_bAnimate);
    ImGui::BeginDisabled(!m_bAnimate);
    {
        float dRotDeg = ToDeg(m_dRot);
        if (ImGui::SliderFloat("Rotation Speed (deg/s)", &dRotDeg, -360.f, 360.f))
            m_dRot = ToRad(dRotDeg);

        float rotDeg = ToDeg(m_rot);
        if (ImGui::SliderFloat("Current Rotation", &rotDeg, -180.f, 180.f))
            m_rot = ToRad(rotDeg);

        if (ImGui::Button("Reset"))
        {
            m_rot = 0.f;
        }
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void InstancingScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void InstancingScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attachments[2] = {};
    attachments[0].name         = "T_SceneColor";
    attachments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attachments);

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBNoInstancing));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBInstancing));

    m_entityNoInstancingCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh                = m_graph.GetConstantBuffer("CB_Camera");

    LoadProgram(m_graph, "P_NoInstancing", SCENE_PATH("NoInstancingShader.hlsl"));
    LoadProgram(m_graph, "P_Instancing", SCENE_PATH("InstancingShader.hlsl"));

    m_graph.AddRenderPass("Pass_Main")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBNoInstancing::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBInstancing::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBNoInstancing::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBInstancing::kSlot)
        .SetExecutor(Bind<&InstancingScene::Submit_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void InstancingScene::Submit_()
{
    Graphics& gfx = GetGraphics();

    if (m_bDoInstancing)
    {
        gfx.SetProgram(m_graph.GetProgram("P_Instancing"));

        const Int32 numRenderEntities = m_numEntitiesPerRow * m_numEntitiesPerRow;
        if (m_numLastRenderedEntities != numRenderEntities)
        {
            JAM_DESTROY_VERTEX_BUFFER(m_instanceVbh);
            m_instanceVbh = gfx.CreateInstanceBuffer(numRenderEntities, sizeof(Mtx4));
        }

        MutableMemoryView mem = gfx.BeginUpdateBuffer(m_instanceVbh);
        Byte*             ptr = mem.GetPtr();
        for (Int32 i = 0; i < m_numEntitiesPerRow; ++i)
        {
            for (Int32 j = 0; j < m_numEntitiesPerRow; ++j)
            {
                const float t = static_cast<float>(i) * 15.f;

                Vec3 euler  = Vec3::kZero;
                euler.pitch = ToRad(t);
                euler.yaw   = m_rot + ToRad(t);

                Vec3 trans = Vec3::kZero;
                trans.x    = 1.5f * static_cast<float>(j - m_numEntitiesPerRow / 2);
                trans.y    = 1.5f * static_cast<float>(i - m_numEntitiesPerRow / 2);

                const Mtx4 world = Mtx4::MakeSRT(Vec3::kOne, euler, trans).Transpose();
                std::memcpy(ptr, &world, sizeof(Mtx4));
                ptr += sizeof(Mtx4);
            }
        }
        gfx.EndUpdateBuffer(m_instanceVbh);

        gfx.SetVertexBuffer(m_vbh, 0, kAll, m_instanceVbh);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
    }
    else
    {
        gfx.SetProgram(m_graph.GetProgram("P_NoInstancing"));
        gfx.SetVertexBuffer(m_vbh);
        gfx.SetIndexBuffer(m_ibh);

        CBNoInstancing cbEntity = {};
        for (Int32 i = 0; i < m_numEntitiesPerRow; ++i)
        {
            for (Int32 j = 0; j < m_numEntitiesPerRow; ++j)
            {
                const float t = static_cast<float>(i) * 15.f;

                Vec3 euler  = Vec3::kZero;
                euler.pitch = ToRad(t);
                euler.yaw   = m_rot + ToRad(t);

                Vec3 trans = Vec3::kZero;
                trans.x    = 1.5f * static_cast<float>(j - m_numEntitiesPerRow / 2);
                trans.y    = 1.5f * static_cast<float>(i - m_numEntitiesPerRow / 2);

                cbEntity.world = Mtx4::MakeSRT(Vec3::kOne, euler, trans).Transpose();
                gfx.UpdateBuffer(m_entityNoInstancingCbh, AsMemoryView(cbEntity));

                gfx.Submit();
            }
        }
    }
    m_numLastRenderedEntities = m_numEntitiesPerRow * m_numEntitiesPerRow;
}
