#include "pch.h"
#include "MultiVertexStreamScene.h"

#include "Gui.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBMultiVertexStreamSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world = Mtx4::kIdentity;
};

struct CBMultiVertexStreamSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos   = Vec3::kZero;
    float padding  = 0.f;
    Mtx4  viewProj = Mtx4::kIdentity;
};

}   // namespace

MultiVertexStreamScene::MultiVertexStreamScene()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/MultiVertexStreamScene/MultiVertexStreamSceneShader.hlsl", vsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/MultiVertexStreamScene/MultiVertexStreamSceneShader.hlsl", psInfo);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

    m_ph = gfx.CreateProgram(vsh, psh, true);

    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBMultiVertexStreamSceneEntity));
    m_cmrCbh    = gfx.CreateConstantBuffer(sizeof(CBMultiVertexStreamSceneCamera));

    struct Vertex
    {
        Vec3 point;
        RGBA rgba;
    };

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
    posLayout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);

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
    colorLayout.Push(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

    UInt16 idxes[] = { 4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 0, 4, 7, 0, 7, 3, 5, 1, 2, 5, 2, 6, 1, 5, 4, 1, 4, 0, 7, 6, 2, 7, 2, 3 };

    m_posVbh   = gfx.CreateVertexBuffer(positions, posLayout);
    m_colorVbh = gfx.CreateVertexBuffer(colors, colorLayout);
    m_ibh      = gfx.CreateIndexBuffer(idxes);

    // camera setting
    m_camera.trans.SetTranslationZ(-25.f);
}

MultiVertexStreamScene::~MultiVertexStreamScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_VERTEX_BUFFER(m_posVbh);
    JAM_DESTROY_VERTEX_BUFFER(m_colorVbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
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
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(_view.vp);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    CBMultiVertexStreamSceneCamera cbCamera = {};
    cbCamera.eyePos                         = m_camera.GetEyePos();
    cbCamera.viewProj                       = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBMultiVertexStreamSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBMultiVertexStreamSceneCamera::kSlot);

    // entity
    gfx.SetProgram(m_ph);
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBMultiVertexStreamSceneEntity::kSlot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBMultiVertexStreamSceneEntity::kSlot);

    VertexStream binginds[] = {
        {   m_posVbh, 0 },
        { m_colorVbh, 0 }
    };
    gfx.SetVertexBuffers(binginds);
    gfx.SetIndexBuffer(m_ibh);

    constexpr Int32 kNumEntitiesPerRow = 11;

    CBMultiVertexStreamSceneEntity cbEntity = {};
    for (Int32 i = 0; i < 11; ++i)
    {
        for (Int32 j = 0; j < 11; ++j)
        {
            const float t = static_cast<float>(i) * 15.f;

            Vec3 euler  = Vec3::kZero;
            euler.pitch = ToRad(t);
            euler.yaw   = m_rot + ToRad(t);

            Vec3 trans = Vec3::kZero;
            trans.x    = 1.5f * static_cast<float>(j - kNumEntitiesPerRow / 2);
            trans.y    = 1.5f * static_cast<float>(i - kNumEntitiesPerRow / 2);

            cbEntity.world = Mtx4::CreateSRT(Vec3::kOne, euler, trans).Transpose();
            gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

            gfx.Submit();
        }
    }

    // blit inner framebuffer texture to output texture
    // output framebuffer maybe do not have depth texture.
    // so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    // in real case. use    post-process pass to do this.
    const FrameBufferInfo fbInfo = gfx.GetFrameBufferInfo(_view.fbh);
    JAM_ASSERT(fbInfo.numRenderTargets > 0, "Back buffer must have at least one render target");
    gfx.CopyTexture(fbInfo.renderTargetTexhs[0], 0, 0, 0, 0, 0, m_fbTexh, 0, 0, 0, 0, 0);
}

void MultiVertexStreamScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("MultiVertexStreamScene"))
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
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    Graphics& gfx = GetGraphics();
    m_fbh         = gfx.CreateFrameBuffer(_width, _height, eTextureFormat::RGBA8_UNorm, eTextureFormat::D24_UNorm_S8_UInt);
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
