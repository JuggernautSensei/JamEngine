#include "pch.h"
#include "SimpleScene.h"

#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBSimpleSceneEntity
{
    static constexpr Int32 k_slot = 0;

    Mtx4  cb_world         = Numbers<Mtx4>::k_identity;
    Mtx4  cb_worldInvTrans = Numbers<Mtx4>::k_identity;
    Vec3  cb_baseColor     = Numbers<Vec3>::k_one;
    float cb_shiness       = 16.f;
};

struct CBSimpleSceneCamera
{
    static constexpr Int32 k_slot = 1;

    Vec3  cb_eyePos   = Numbers<Vec3>::k_zero;
    float padding1    = 0.f;
    Mtx4  cb_viewProj = Numbers<Mtx4>::k_identity;
};

struct CBSimpleSceneLight
{
    static constexpr Int32 k_slot = 2;

    Vec3  cb_lightDir     = { 0.f, -1.f, 0.f };
    float padding2        = 0.f;
    Vec3  cb_lightColor   = Numbers<Vec3>::k_one;
    float padding3        = 0.f;
    Vec3  cb_ambientColor = Numbers<Vec3>::k_one;
    float padding4        = 0.f;
};

struct SimpleSceneVertex
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

struct SimpleSceneMisc
{
    static constexpr Int32 k_slot = 3;

    Int32 debugNormal  = 0;
    Int32 bEnableLight = 1;
    Int32 padding1     = 0;
    Int32 padding2     = 0;
};

}   // namespace

SimpleScene::SimpleScene()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Shaders/SimpleSceneShader.hlsl", vsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Shaders/SimpleSceneShader.hlsl", psInfo);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

    m_ph = gfx.CreateProgram(vsh, psh, true);

    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneEntity));
    m_cmrCbh    = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneLight));
    m_miscCbh   = gfx.CreateConstantBuffer(sizeof(SimpleSceneMisc));

    m_pMesh[eMeshType::Cube]   = CreateCubeMesh(Vec3 { 0.5f }, SimpleSceneVertex::s_layout);
    m_pMesh[eMeshType::Sphere] = CreateSphereMesh(0.5f, 5, SimpleSceneVertex::s_layout);

    // load model
    AssimpImporterInfo info         = {};
    info.bGenerateNormals           = true;
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("StanfordBunny.obj", info);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModel = importer->ExtractToModel(SimpleSceneVertex::s_layout);
    JAM_ASSERT(pModel, "Failed to extract model: {}", pModel.GetError().ToErrorMessage());
    Ref<Model> pBunnyModel    = pModel.GetValue();
    m_pMesh[eMeshType::Bunny] = pBunnyModel->GetMeshes()[0];

    // camera setting
    m_camera.SetAspectRatio(16.f / 9.f);
    m_cameraTransform.SetTranslationZ(-8.f);
}

SimpleScene::~SimpleScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_miscCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
}

void SimpleScene::OnUpdate(
    const float _deltaTime)
{
    if (m_bAnimate)
    {
        m_entityYRad += m_dMoveRad * _deltaTime;
        m_entityYawRad += m_dYawRad * _deltaTime;
    }
}

void SimpleScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearColor(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(m_vp);
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);

    // camera
    const Mtx4          vpMtx    = m_cameraTransform.CreateViewMatrix() * m_camera.GetProjMatrix();
    CBSimpleSceneCamera cbCamera = {};
    cbCamera.cb_eyePos           = m_cameraTransform.GetTranslation();
    cbCamera.cb_viewProj         = vpMtx.Transpose();
    gfx.UploadBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBSimpleSceneCamera::k_slot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBSimpleSceneCamera::k_slot);

    // light
    CBSimpleSceneLight cbLight = {};
    cbLight.cb_lightDir        = Normalized(m_lightLookAt - m_lightPos);
    cbLight.cb_lightColor      = m_lightColor;
    cbLight.cb_ambientColor    = m_ambientColor;
    gfx.UploadBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBSimpleSceneLight::k_slot);

    // misc
    SimpleSceneMisc cbMisc = {};
    cbMisc.debugNormal     = 0;
    cbMisc.bEnableLight    = 1;
    gfx.UploadBuffer(m_miscCbh, AsMemoryView(cbMisc));
    gfx.SetConstantBuffer(m_miscCbh, eShader::PixelShader, SimpleSceneMisc::k_slot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBSimpleSceneEntity::k_slot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBSimpleSceneEntity::k_slot);

    CBSimpleSceneEntity cbEntity = {};
    Ref<Mesh>&          pMesh    = m_pMesh[m_activeType];
    for (Int32 i = 0; i < m_numEntities; ++i)
    {
        float t   = NumericCast<float>(i) * 15.f;
        float yaw = m_entityYawRad + ToRad(t);
        float x   = 1.f * NumericCast<float>(i - m_numEntities / 2);
        float y   = 1.f * Sin(m_entityYRad + ToRad(t));
        float s   = m_activeType == eMeshType::Bunny ? 5.f : 1.f;   // bunny is too small..

        const Mtx4 world          = Mtx4::CreateScale(s, s, s) * Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
        cbEntity.cb_world         = world.Transpose();
        cbEntity.cb_worldInvTrans = world.Invert();
        cbEntity.cb_shiness       = m_shiness;
        cbEntity.cb_baseColor     = m_baseColor;
        gfx.UploadBuffer(m_entityCbh, AsMemoryView(cbEntity));
        pMesh->Submit();
    }

    // blit inner framebuffer texture to output texture
    // output framebuffer maybe do not have depth texture.
    // so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    // in real case. use    post-process pass to do this.
    const FrameBufferInfo fbInfo = gfx.GetFrameBufferInfo(_view.outputFbh);
    JAM_ASSERT(fbInfo.numRenderTargets > 0, "Back buffer must have at least one render target");
    gfx.CopyTexture(fbInfo.renderTargetTexhs[0], m_fbTexh);
}

void SimpleScene::OnImGuiRender()
{
    ImGui::Begin("SimpleScene Controller");

    ImGui::Text("FPS: %.2f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();

    ImGui::Text("Entity Selection");
    if (ImGui::RadioButton("Cube", m_activeType == eMeshType::Cube))
    {
        m_activeType = eMeshType::Cube;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Sphere", m_activeType == eMeshType::Sphere))
    {
        m_activeType = eMeshType::Sphere;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Bunny", m_activeType == eMeshType::Bunny))
    {
        m_activeType = eMeshType::Bunny;
    }

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

void SimpleScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    Graphics& gfx = GetGraphics();
    m_fbh         = gfx.CreateFrameBuffer(_width, _height, eTextureFormat::RGBA8_UNorm, eTextureFormat::D24_UNorm_S8_UInt);
    m_vp          = Viewport { 0, 0, NumericCast<UInt16>(_width), NumericCast<UInt16>(_height) };

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
