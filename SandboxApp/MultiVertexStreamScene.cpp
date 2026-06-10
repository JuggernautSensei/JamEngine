#include "pch.h"
#include "MultiVertexStreamScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"

#define SCENE_PATH(_filepath) "Resources/MultiVertexStreamScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world;
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding;
    Mtx4  viewProj;
};

}   // namespace

MultiVertexStreamScene::MultiVertexStreamScene()
{
    InitMeshes_();
    InitRenderGraph_();

    m_camera.transform.SetTranslationZ(-25.f);
}

MultiVertexStreamScene::~MultiVertexStreamScene()
{
    JAM_DESTROY_VERTEX_BUFFER(m_posVbh);
    JAM_DESTROY_VERTEX_BUFFER(m_colorVbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
}

void MultiVertexStreamScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_rot += m_dRot * _deltaTime;
    }
}

void MultiVertexStreamScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eyePos   = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void MultiVertexStreamScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("MultiVertexStreamScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::End();
}

void MultiVertexStreamScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void MultiVertexStreamScene::InitMeshes_()
{
    Graphics& gfx = GetGraphics();

    Vec3 positions[] = {
        { -0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f }
    };

    VertexLayout posLayout = {};
    posLayout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);

    RGBA colors[] = {
        RGBA::FromLinear(1.0f, 0.0f, 0.0f, 1.0f),
        RGBA::FromLinear(0.0f, 1.0f, 0.0f, 1.0f),
        RGBA::FromLinear(0.0f, 0.0f, 1.0f, 1.0f),
        RGBA::FromLinear(1.0f, 1.0f, 0.0f, 1.0f),
        RGBA::FromLinear(1.0f, 0.0f, 1.0f, 1.0f),
        RGBA::FromLinear(0.0f, 1.0f, 1.0f, 1.0f),
        RGBA::FromLinear(1.0f, 0.5f, 0.0f, 1.0f),
        RGBA::FromLinear(1.0f, 0.0f, 0.5f, 1.0f)
    };

    VertexLayout colorLayout = {};
    colorLayout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

    UInt16 idxes[] = { 4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 0, 4, 7, 0, 7, 3, 5, 1, 2, 5, 2, 6, 1, 5, 4, 1, 4, 0, 7, 6, 2, 7, 2, 3 };

    m_posVbh   = gfx.CreateVertexBuffer(positions, posLayout);
    m_colorVbh = gfx.CreateVertexBuffer(colors, colorLayout);
    m_ibh      = gfx.CreateIndexBuffer(idxes);
}

void MultiVertexStreamScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attachments[2] = {};
    attachments[0].name         = "T_SceneColor";
    attachments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attachments);

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");

    LoadProgram(m_graph, "P_Main", SCENE_PATH("MultiVertexStreamShader.hlsl"));

    m_graph.AddRenderPass("Pass_Main")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Main")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetExecutor(Bind<&MultiVertexStreamScene::Submit_>(this));

    ImageFilterManager& ifm      = GetImageFilterManager();
    ImageFilterDesc     blitDesc = {};
    blitDesc.filter              = eImageFilter::Blit;
    blitDesc.output              = eImageFilterOutput::Final;
    blitDesc.input0Texture       = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitDesc);
}

void MultiVertexStreamScene::Submit_() const
{
    constexpr Int32 kNumEntitiesPerRow = 11;

    Graphics& gfx = GetGraphics();

    VertexStream bindings[] = {
        {   m_posVbh, 0 },
        { m_colorVbh, 0 }
    };

    gfx.SetVertexBuffers(bindings);
    gfx.SetIndexBuffer(m_ibh);

    CBEntity entity;
    for (Int32 i = 0; i < kNumEntitiesPerRow; ++i)
    {
        for (Int32 j = 0; j < kNumEntitiesPerRow; ++j)
        {
            const float t = static_cast<float>(i) * 15.f;

            Vec3 euler  = Vec3::kZero;
            euler.pitch = ToRad(t);
            euler.yaw   = m_rot + ToRad(t);

            Vec3 trans = Vec3::kZero;
            trans.x    = 1.5f * static_cast<float>(j - kNumEntitiesPerRow / 2);
            trans.y    = 1.5f * static_cast<float>(i - kNumEntitiesPerRow / 2);

            entity.world = Mtx4::MakeSRT(Vec3::kOne, euler, trans).Transpose();
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

            gfx.Submit();
        }
    }
}
