#include "pch.h"
#include "InstancingScene.h"

namespace
{

struct CBInstancingSceneEntity_NoInstancing
{
    static constexpr Int32 k_slot = 0;

    Mtx4  cb_world         = Numbers<Mtx4>::k_identity;
    Mtx4  cb_worldInvTrans = Numbers<Mtx4>::k_identity;
    Vec3  cb_baseColor     = Numbers<Vec3>::k_one;
    float cb_shiness       = 16.f;
};

struct CBInstancingSceneEntity_Instancing
{
    static constexpr Int32 k_slot       = 0;
    Vec3                   cb_baseColor = Numbers<Vec3>::k_one;
    float                  cb_shiness   = 16.f;
};

struct CBInstancingSceneCamera
{
    static constexpr Int32 k_slot = 1;

    Vec3  cb_eyePos   = Numbers<Vec3>::k_zero;
    float padding1    = 0.f;
    Mtx4  cb_viewProj = Numbers<Mtx4>::k_identity;
};

struct CBInstancingSceneLight
{
    static constexpr Int32 k_slot = 2;

    Vec3  cb_lightDir     = { 0.f, -1.f, 0.f };
    float padding2        = 0.f;
    Vec3  cb_lightColor   = Numbers<Vec3>::k_one;
    float padding3        = 0.f;
    Vec3  cb_ambientColor = Numbers<Vec3>::k_one;
    float padding4        = 0.f;
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
    m_entityInstancingCbh   = gfx.CreateConstantBuffer(sizeof(CBInstancingSceneEntity_Instancing));
    m_cmrCbh                = gfx.CreateConstantBuffer(sizeof(CBInstancingSceneCamera));
    m_lightCbh              = gfx.CreateConstantBuffer(sizeof(CBInstancingSceneLight));

    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
    m_pMesh = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);

    // camera setting
    m_camera.SetAspectRatio(16.f / 9.f);
    m_cameraTransform.SetTranslationZ(-50.f);
}

InstancingScene::~InstancingScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_PROGRAM(m_instPh);
    JAM_DESTROY_VERTEX_BUFFER(m_instbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityNoInstancingCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityInstancingCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
}

void InstancingScene::OnUpdate(
    const float _deltaTime)
{
    if (m_bAnimate)
    {
        m_entityYRad += m_dMoveRad * _deltaTime;
        m_entityYawRad += m_dYawRad * _deltaTime;
    }
}

void InstancingScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(m_vp);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    const Mtx4              vpMtx    = m_cameraTransform.GetMatrix().Invert() * m_camera.GetProjMatrix();
    CBInstancingSceneCamera cbCamera = {};
    cbCamera.cb_eyePos               = m_cameraTransform.GetTranslation();
    cbCamera.cb_viewProj             = vpMtx.Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBInstancingSceneCamera::k_slot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBInstancingSceneCamera::k_slot);

    // light
    CBInstancingSceneLight cbLight = {};
    cbLight.cb_lightDir            = Normalized(m_lightLookAt - m_lightPos);
    cbLight.cb_lightColor          = m_lightColor;
    cbLight.cb_ambientColor        = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBInstancingSceneLight::k_slot);

    // entity
    if (m_bDoInstancing)
    {
        gfx.SetProgram(m_instPh);

        const Int32 numRenderEntities = m_numRows * m_numEntitiesPerRow;
        if (m_numLastRenderedEntities != numRenderEntities)
        {
            // recreate instance buffer
            JAM_DESTROY_VERTEX_BUFFER(m_instbh);
            m_instbh = gfx.CreateInstanceBuffer(numRenderEntities, sizeof(Mtx4) * 2);
        }

        MutableMemoryView mem = gfx.BeginUpdateBuffer(m_instbh);
        Byte*             ptr = mem.GetPtr();
        for (Int32 i = 0; i < m_numRows; ++i)
        {
            for (Int32 j = 0; j < m_numEntitiesPerRow; ++j)
            {
                float      t             = static_cast<float>(i) * 15.f;
                float      yaw           = m_entityYawRad + ToRad(t);
                float      pitch         = ToRad(t);
                float      x             = 1.f * static_cast<float>(j - m_numEntitiesPerRow / 2);
                float      y             = 1.f * static_cast<float>(i - m_numRows / 2);
                const Mtx4 world         = Mtx4::CreateFromYawPitchRoll(yaw, pitch, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
                const Mtx4 worldTrans    = world.Transpose();
                const Mtx4 worldInvTrans = world.Invert();
                ::memcpy(ptr, &worldTrans, sizeof(Mtx4));
                ptr = ptr + sizeof(Mtx4);
                ::memcpy(ptr, &worldInvTrans, sizeof(Mtx4));
                ptr = ptr + sizeof(Mtx4);
            }
        }
        gfx.EndUpdateBuffer(m_instbh);

        CBInstancingSceneEntity_Instancing cbEntity = {};
        cbEntity.cb_shiness                         = m_shiness;
        cbEntity.cb_baseColor                       = m_baseColor;
        gfx.UpdateBuffer(m_entityInstancingCbh, AsMemoryView(cbEntity));
        gfx.SetConstantBuffer(m_entityInstancingCbh, eShader::PixelShader, CBInstancingSceneEntity_Instancing::k_slot);

        gfx.SetInstanceBuffer(m_instbh);
        m_pMesh->Submit();
    }
    else
    {
        gfx.SetProgram(m_ph);

        gfx.SetConstantBuffer(m_entityNoInstancingCbh, eShader::VertexShader, CBInstancingSceneEntity_NoInstancing::k_slot);
        gfx.SetConstantBuffer(m_entityNoInstancingCbh, eShader::PixelShader, CBInstancingSceneEntity_NoInstancing::k_slot);

        CBInstancingSceneEntity_NoInstancing cbEntity = {};
        for (Int32 i = 0; i < m_numRows; ++i)
        {
            for (Int32 j = 0; j < m_numEntitiesPerRow; ++j)
            {
                float t     = static_cast<float>(i) * 15.f;
                float yaw   = m_entityYawRad + ToRad(t);
                float pitch = ToRad(t);
                float x     = 1.f * static_cast<float>(j - m_numEntitiesPerRow / 2);
                float y     = 1.f * static_cast<float>(i - m_numRows / 2);

                const Mtx4 world          = Mtx4::CreateFromYawPitchRoll(yaw, pitch, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
                cbEntity.cb_world         = world.Transpose();
                cbEntity.cb_worldInvTrans = world.Invert();
                cbEntity.cb_shiness       = m_shiness;
                cbEntity.cb_baseColor     = m_baseColor;
                gfx.UpdateBuffer(m_entityNoInstancingCbh, AsMemoryView(cbEntity));
                m_pMesh->Submit();
            }
        }
    }
    m_numLastRenderedEntities = m_numRows * m_numEntitiesPerRow;

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
    ImGui::Begin("InstancingScene Controller");

    // 1. 성능 정보 및 모드 설정
    ImGui::Text("Performance");
    ImGui::Text("FPS: %.2f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Total Entities: %d", m_numRows * m_numEntitiesPerRow);
    ImGui::Text("Draw Calls: %d", GetGraphics().GetStats().numDrawCalls);

    ImGui::Separator();

    // 인스턴싱 사용 여부 토글
    ImGui::Checkbox("Use GPU Instancing", &m_bDoInstancing);
    ImGui::Separator();

    // 2. 엔티티 배치 설정 (행/열 제어)
    if (ImGui::CollapsingHeader("Entity Grid Setup", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Rotation", &m_bAnimate);

        // 헤더에 정의된 m_numRows와 m_numEntitiesPerRow 사용
        ImGui::SliderInt("Rows", &m_numRows, 1, 200);
        ImGui::SliderInt("Entities Per Row", &m_numEntitiesPerRow, 1, 200);

        if (ImGui::Button("Reset Grid"))
        {
            m_numRows           = 10;
            m_numEntitiesPerRow = 10;
        }
    }

    // 3. 재질 및 조명 설정
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

    // 4. 카메라 정보
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

void InstancingScene::OnResize(
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
