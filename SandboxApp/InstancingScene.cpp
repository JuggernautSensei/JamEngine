#include "pch.h"
#include "InstancingScene.h"

#include "Gui.h"

namespace
{

struct CBInstancingSceneEntity_NoInstancing
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world;
};

struct CBInstancingSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding1;
    Mtx4  viewProj;
};

}   // namespace

InstancingScene::InstancingScene()
{
    Graphics& gfx = GetGraphics();

    // no instancing
    {
        ShaderCompilerInfo vsInfo = {};
        vsInfo.entryPoint         = "VSMain";
        vsInfo.stage              = eShader::VertexShader;
        const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/InstancingScene/InstancingScene_NoInstancing.hlsl", vsInfo);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        ShaderCompilerInfo psInfo = {};
        psInfo.entryPoint         = "PSMain";
        psInfo.stage              = eShader::PixelShader;
        const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/InstancingScene/InstancingScene_NoInstancing.hlsl", psInfo);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_ph = gfx.CreateProgram(vsh, psh, true);
    }

    // instancing
    {
        ShaderCompilerInfo vsInfo = {};
        vsInfo.entryPoint         = "VSMain";
        vsInfo.stage              = eShader::VertexShader;
        const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/InstancingScene/InstancingScene_Instancing.hlsl", vsInfo);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        ShaderCompilerInfo psInfo = {};
        psInfo.entryPoint         = "PSMain";
        psInfo.stage              = eShader::PixelShader;
        const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/InstancingScene/InstancingScene_Instancing.hlsl", psInfo);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_instPh = gfx.CreateProgram(vsh, psh, true);
    }

    m_entityNoInstancingCbh = gfx.CreateConstantBuffer(sizeof(CBInstancingSceneEntity_NoInstancing));
    m_cmrCbh                = gfx.CreateConstantBuffer(sizeof(CBInstancingSceneCamera));

    struct Vertex
    {
        Vec3 point;
        RGBA rgba;
    };

    VertexLayout layout = {};
    layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
    layout.Push(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

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

    m_camera.trans.SetTranslationZ(-25.f);
}

InstancingScene::~InstancingScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_PROGRAM(m_instPh);
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_VERTEX_BUFFER(m_instanceVbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityNoInstancingCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
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
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(_view.vp);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    CBInstancingSceneCamera cbCamera = {};
    cbCamera.eyePos                  = m_camera.GetEyePos();
    cbCamera.viewProj                = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBInstancingSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBInstancingSceneCamera::kSlot);

    // entity
    if (m_bDoInstancing)
    {
        gfx.SetProgram(m_instPh);

        const Int32 numRenderEntities = m_numEntitiesPerRow * m_numEntitiesPerRow;
        if (m_numLastRenderedEntities != numRenderEntities)
        {
            // recreate instance buffer
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

                const Mtx4 world = Mtx4::CreateSRT(Vec3::kOne, euler, trans).Transpose();
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
        gfx.SetProgram(m_ph);

        gfx.SetConstantBuffer(m_entityNoInstancingCbh, eShader::VertexShader, CBInstancingSceneEntity_NoInstancing::kSlot);
        gfx.SetConstantBuffer(m_entityNoInstancingCbh, eShader::PixelShader, CBInstancingSceneEntity_NoInstancing::kSlot);

        gfx.SetVertexBuffer(m_vbh);
        gfx.SetIndexBuffer(m_ibh);

        CBInstancingSceneEntity_NoInstancing cbEntity = {};
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

                cbEntity.world = Mtx4::CreateSRT(Vec3::kOne, euler, trans).Transpose();
                gfx.UpdateBuffer(m_entityNoInstancingCbh, AsMemoryView(cbEntity));

                gfx.Submit();
            }
        }
    }
    m_numLastRenderedEntities = m_numEntitiesPerRow * m_numEntitiesPerRow;

    // blit inner framebuffer texture to output texture
    // output framebuffer maybe do not have depth texture.
    // so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    // in real case. use    post-process pass to do this.
    const FrameBufferInfo fbInfo = gfx.GetFrameBufferInfo(_view.fbh);
    JAM_ASSERT(fbInfo.numRenderTargets > 0, "Back buffer must have at least one render target");
    gfx.CopyTexture(fbInfo.renderTargetTexhs[0], 0, 0, 0, 0, 0, m_fbTexh, 0, 0, 0, 0, 0);
}

void InstancingScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("InstancingScene"))
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
            m_rot     = 0.f;
        }
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void InstancingScene::OnResize(
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
