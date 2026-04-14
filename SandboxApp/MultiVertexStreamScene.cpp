#include "pch.h"
#include "MultiVertexStreamScene.h"

#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBMultiVertexStreamSceneEntity
{
    static constexpr Int32 k_slot = 0;

    Mtx4  cb_world         = Numbers<Mtx4>::k_identity;
    Mtx4  cb_worldInvTrans = Numbers<Mtx4>::k_identity;
    Vec3  cb_baseColor     = Numbers<Vec3>::k_one;
    float cb_shiness       = 16.f;
};

struct CBMultiVertexStreamSceneCamera
{
    static constexpr Int32 k_slot = 1;

    Vec3  cb_eyePos   = Numbers<Vec3>::k_zero;
    float padding1    = 0.f;
    Mtx4  cb_viewProj = Numbers<Mtx4>::k_identity;
};

struct CBMultiVertexStreamSceneLight
{
    static constexpr Int32 k_slot = 2;

    Vec3  cb_lightDir     = { 0.f, -1.f, 0.f };
    float padding2        = 0.f;
    Vec3  cb_lightColor   = Numbers<Vec3>::k_one;
    float padding3        = 0.f;
    Vec3  cb_ambientColor = Numbers<Vec3>::k_one;
    float padding4        = 0.f;
};

struct MultiVertexStreamScenePosNormalVertex
{
    Vec3 pos    = Numbers<Vec3>::k_zero;
    Vec3 normal = Numbers<Vec3>::k_zero;

    inline static VertexLayout s_layout = [] {
        VertexLayout layout = {};
        layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
        return layout;
    }();
};

struct MultiVertexStreamSceneColorVertex
{
    Vec3 color = Numbers<Vec3>::k_zero;

    inline static VertexLayout s_layout = [] {
        VertexLayout layout = {};
        layout.Push(eVertexAttribute::Color0, eAttributeFormat::Float3);
        return layout;
    }();
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
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBMultiVertexStreamSceneLight));

    MultiVertexStreamScenePosNormalVertex posNormals[24] = {
        {  { -0.5f, 0.5f, -0.5f }, { 0.f, 0.f, -1.f } },
        {   { 0.5f, 0.5f, -0.5f }, { 0.f, 0.f, -1.f } },
        {  { 0.5f, -0.5f, -0.5f }, { 0.f, 0.f, -1.f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.f, 0.f, -1.f } },

        {   { 0.5f, 0.5f, -0.5f },  { 1.f, 0.f, 0.f } },
        {    { 0.5f, 0.5f, 0.5f },  { 1.f, 0.f, 0.f } },
        {   { 0.5f, -0.5f, 0.5f },  { 1.f, 0.f, 0.f } },
        {  { 0.5f, -0.5f, -0.5f },  { 1.f, 0.f, 0.f } },

        {    { 0.5f, 0.5f, 0.5f },  { 0.f, 0.f, 1.f } },
        {   { -0.5f, 0.5f, 0.5f },  { 0.f, 0.f, 1.f } },
        {  { -0.5f, -0.5f, 0.5f },  { 0.f, 0.f, 1.f } },
        {   { 0.5f, -0.5f, 0.5f },  { 0.f, 0.f, 1.f } },

        {   { -0.5f, 0.5f, 0.5f }, { -1.f, 0.f, 0.f } },
        {  { -0.5f, 0.5f, -0.5f }, { -1.f, 0.f, 0.f } },
        { { -0.5f, -0.5f, -0.5f }, { -1.f, 0.f, 0.f } },
        {  { -0.5f, -0.5f, 0.5f }, { -1.f, 0.f, 0.f } },

        {   { -0.5f, 0.5f, 0.5f },  { 0.f, 1.f, 0.f } },
        {    { 0.5f, 0.5f, 0.5f },  { 0.f, 1.f, 0.f } },
        {   { 0.5f, 0.5f, -0.5f },  { 0.f, 1.f, 0.f } },
        {  { -0.5f, 0.5f, -0.5f },  { 0.f, 1.f, 0.f } },

        { { -0.5f, -0.5f, -0.5f }, { 0.f, -1.f, 0.f } },
        {  { 0.5f, -0.5f, -0.5f }, { 0.f, -1.f, 0.f } },
        {   { 0.5f, -0.5f, 0.5f }, { 0.f, -1.f, 0.f } },
        {  { -0.5f, -0.5f, 0.5f }, { 0.f, -1.f, 0.f } },
    };

    MultiVertexStreamSceneColorVertex colors[24] = {
        { { 1.f, 0.f, 0.f } },
        { { 1.f, 0.f, 0.f } },
        { { 1.f, 0.f, 0.f } },
        { { 1.f, 0.f, 0.f } },
        { { 0.f, 1.f, 0.f } },
        { { 0.f, 1.f, 0.f } },
        { { 0.f, 1.f, 0.f } },
        { { 0.f, 1.f, 0.f } },
        { { 0.f, 0.f, 1.f } },
        { { 0.f, 0.f, 1.f } },
        { { 0.f, 0.f, 1.f } },
        { { 0.f, 0.f, 1.f } },
        { { 1.f, 1.f, 0.f } },
        { { 1.f, 1.f, 0.f } },
        { { 1.f, 1.f, 0.f } },
        { { 1.f, 1.f, 0.f } },
        { { 1.f, 0.f, 1.f } },
        { { 1.f, 0.f, 1.f } },
        { { 1.f, 0.f, 1.f } },
        { { 1.f, 0.f, 1.f } },
        { { 0.f, 1.f, 1.f } },
        { { 0.f, 1.f, 1.f } },
        { { 0.f, 1.f, 1.f } },
        { { 0.f, 1.f, 1.f } },
    };

    UInt16 indices[36] = {
        0, 1, 2, 0, 2, 3,   // Front
        4,
        5,
        6,
        4,
        6,
        7,   // Right
        8,
        9,
        10,
        8,
        10,
        11,   // Back
        12,
        13,
        14,
        12,
        14,
        15,   // Left
        16,
        17,
        18,
        16,
        18,
        19,   // Top
        20,
        21,
        22,
        20,
        22,
        23   // Bottom
    };

    m_posNormalVbh = gfx.CreateVertexBuffer(posNormals, MultiVertexStreamScenePosNormalVertex::s_layout);
    m_colorVbh     = gfx.CreateVertexBuffer(colors, MultiVertexStreamSceneColorVertex::s_layout);
    m_ibh          = gfx.CreateIndexBuffer(indices);

    // camera setting
    m_camera.SetAspectRatio(16.f / 9.f);
    m_cameraTransform.SetTranslationZ(-8.f);
}

MultiVertexStreamScene::~MultiVertexStreamScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_VERTEX_BUFFER(m_posNormalVbh);
    JAM_DESTROY_VERTEX_BUFFER(m_colorVbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
}

void MultiVertexStreamScene::OnUpdate(
    const float _deltaTime)
{
    if (m_bAnimate)
    {
        m_entityYRad += m_dMoveRad * _deltaTime;
        m_entityYawRad += m_dYawRad * _deltaTime;
    }
}

void MultiVertexStreamScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(m_vp);
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    const Mtx4                     vpMtx    = m_cameraTransform.GetMatrix().Invert() * m_camera.GetProjMatrix();
    CBMultiVertexStreamSceneCamera cbCamera = {};
    cbCamera.cb_eyePos                      = m_cameraTransform.GetTranslation();
    cbCamera.cb_viewProj                    = vpMtx.Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBMultiVertexStreamSceneCamera::k_slot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBMultiVertexStreamSceneCamera::k_slot);

    // light
    CBMultiVertexStreamSceneLight cbLight = {};
    cbLight.cb_lightDir                   = Normalized(m_lightLookAt - m_lightPos);
    cbLight.cb_lightColor                 = m_lightColor;
    cbLight.cb_ambientColor               = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBMultiVertexStreamSceneLight::k_slot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBMultiVertexStreamSceneEntity::k_slot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBMultiVertexStreamSceneEntity::k_slot);

    CBMultiVertexStreamSceneEntity cbEntity = {};
    for (Int32 i = 0; i < m_numEntities; ++i)
    {
        float t   = static_cast<float>(i) * 15.f;
        float yaw = m_entityYawRad + ToRad(t);
        float x   = 1.f * static_cast<float>(i - (m_numEntities / 2));
        float y   = 1.f * Sin(m_entityYRad + ToRad(t));

        const Mtx4 world          = Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
        cbEntity.cb_world         = world.Transpose();
        cbEntity.cb_worldInvTrans = world.Invert();
        cbEntity.cb_shiness       = m_shiness;
        cbEntity.cb_baseColor     = m_baseColor;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));

        gfx.SetVertexBuffer(m_posNormalVbh);
        gfx.SetVertexBuffer(m_colorVbh);
        gfx.SetIndexBuffer(m_ibh);
        gfx.Submit();
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
    ImGui::Begin("MultiVertexStreamScene Controller");

    ImGui::Text("FPS: %.2f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Entity Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Rotation", &m_bAnimate);

        ImGui::SliderInt("Entities", &m_numEntities, 1, 100);
    }

    if (ImGui::CollapsingHeader("Material & Light"))
    {
        ImGui::ColorEdit3("Base Color", &m_baseColor.x);
        ImGui::SliderFloat("Shininess", &m_shiness, 1.0f, 256.0f);

        ImGui::Separator();
        ImGui::DragFloat3("Light Pos", &m_lightPos.x, 0.1f);
        ImGui::DragFloat3("Light LookAt", &m_lightLookAt.x, 0.1f);
        ImGui::ColorEdit3("Light Color", &m_lightColor.x);
        ImGui::ColorEdit3("Ambient Color", &m_ambientColor.x);
    }

    // 5. 카메라 정보 (Camera)
    if (ImGui::CollapsingHeader("Camera"))
    {
        Vec3 camPos = m_cameraTransform.GetTranslation();
        ImGui::Text("Camera Pos: %.2f, %.2f, %.2f", camPos.x, camPos.y, camPos.z);

        float fov = m_camera.GetFovYRad();
        if (ImGui::SliderAngle("Field of View", &fov, 10.0f, 120.0f))
        {
            m_camera.SetFovYRad(fov);
        }
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
    m_vp          = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
