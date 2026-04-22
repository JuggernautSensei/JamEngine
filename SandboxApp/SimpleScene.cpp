#include "pch.h"
#include "SimpleScene.h"

#include "Gui.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

struct CBSimpleSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world         = Mtx4::kIdentity;
    Mtx4 invTransWorld = Mtx4::kIdentity;
    Vec3   baseColor     = Vec3::kOne;
    float  shiness       = 16.f;
};

struct CBSimpleSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3   eye      = Vec3::kZero;
    float  padding1 = 0.f;
    Mtx4 viewProj = Mtx4::kIdentity;
};

struct CBSimpleSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightDir   = { 0.f, -1.f, 0.f };
    float padding2   = 0.f;
    Vec3  lightCol   = Vec3::kOne;
    float padding3   = 0.f;
    Vec3  ambientCol = Vec3::kOne;
    float padding4   = 0.f;
};

struct SimpleSceneVertex
{
    Vec3 pos    = Vec3::kZero;
    Vec3 normal = Vec3::kZero;

    inline static VertexLayout s_attribLayout = [] {
        VertexLayout layout = {};
        layout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
        return layout;
    }();
};

}   // namespace

SimpleScene::SimpleScene()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/SimpleScene/SimpleSceneShader.hlsl", vsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/SimpleScene/SimpleSceneShader.hlsl", psInfo);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

    m_ph = gfx.CreateProgram(vsh, psh, true);

    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneEntity));
    m_cmrCbh    = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBSimpleSceneLight));

    m_pMesh[eMesh::Cube]   = CreateCubeMesh(Vec3 { 0.5f }, SimpleSceneVertex::s_attribLayout);
    m_pMesh[eMesh::Sphere] = CreateSphereMesh(0.5f, 5, SimpleSceneVertex::s_attribLayout);

    // load model
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/Models/StanfordBunny.obj", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModel = importer->ConvertToModel(SimpleSceneVertex::s_attribLayout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModel, "Failed to extract model: {}", pModel.GetError().ToErrorMessage());
    Ref<Model> pBunnyModel = pModel.GetValue();
    m_pMesh[eMesh::Bunny]  = pBunnyModel->GetMeshes()[0];

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
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(m_vp);
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    const Mtx4        vpMtx    = m_cameraTransform.GetSRT().Invert() * m_camera.GetProj();
    CBSimpleSceneCamera cbCamera = {};
    cbCamera.eye                 = m_cameraTransform.GetTranslation();
    cbCamera.viewProj            = vpMtx.Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBSimpleSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBSimpleSceneCamera::kSlot);

    // light
    CBSimpleSceneLight cbLight = {};
    cbLight.lightDir           = Normalized(m_lightLookAt - m_lightPos);
    cbLight.lightCol           = m_lightCol;
    cbLight.ambientCol         = m_ambientCol;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBSimpleSceneLight::kSlot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBSimpleSceneEntity::kSlot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBSimpleSceneEntity::kSlot);

    CBSimpleSceneEntity cbEntity = {};
    Ref<Mesh>&          pMesh    = m_pMesh[m_activeMesh];
    for (Int32 i = 0; i < m_numEntities; ++i)
    {
        float t   = static_cast<float>(i) * 15.f;
        float yaw = m_entityYawRad + ToRad(t);
        float x   = 1.f * static_cast<float>(i - (m_numEntities / 2));
        float y   = 1.f * Sin(m_entityYRad + ToRad(t));
        float s   = m_activeMesh == eMesh::Bunny ? 5.f : 1.f;   // bunny is too small..

        const Mtx4 world     = Mtx4::CreateScale(s, s, s) * Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, y, 0.f);
        cbEntity.world         = world.Transpose();
        cbEntity.invTransWorld = world.Invert();
        cbEntity.shiness       = m_shiness;
        cbEntity.baseColor     = m_baseCol;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
        pMesh->Submit();
    }

    // blit inner framebuffer texture to output texture
    // output framebuffer maybe do not have depth texture.
    // so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    // in real case. use    post-process pass to do this.
    const FrameBufferInfo fbInfo = gfx.GetFrameBufferInfo(_view.fbh);
    JAM_ASSERT(fbInfo.numRenderTargets > 0, "Back buffer must have at least one render target");
    gfx.CopyTexture(fbInfo.renderTargetTexhs[0], 0, 0, 0, 0, 0, m_fbTexh, 0, 0, 0, 0, 0);
}

void SimpleScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("SimpleScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Entity Selection");
    if (ImGui::RadioButton("Cube", m_activeMesh == eMesh::Cube))
    {
        m_activeMesh = eMesh::Cube;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Sphere", m_activeMesh == eMesh::Sphere))
    {
        m_activeMesh = eMesh::Sphere;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Bunny", m_activeMesh == eMesh::Bunny))
    {
        m_activeMesh = eMesh::Bunny;
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Entity Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Rotation", &m_bAnimate);

        ImGui::SliderInt("Entities", &m_numEntities, 1, 100);
    }

    if (ImGui::CollapsingHeader("Material & Light"))
    {
        ImGui::ColorEdit3("Base Color", &m_baseCol.x);
        ImGui::SliderFloat("Shininess", &m_shiness, 1.0f, 256.0f);

        ImGui::Separator();
        ImGui::DragFloat3("Light Pos", &m_lightPos.x, 0.1f);
        ImGui::DragFloat3("Light LookAt", &m_lightLookAt.x, 0.1f);
        ImGui::ColorEdit3("Light Color", &m_lightCol.x);
        ImGui::ColorEdit3("Ambient Color", &m_ambientCol.x);
    }

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
    m_vp          = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
