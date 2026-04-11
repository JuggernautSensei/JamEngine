#include "pch.h"
#include "TextureMappingScene.h"

#include "../JamEngine/AssimpImporter.h"

namespace
{

constexpr Int32 k_linearSamplerSlot = 0;
constexpr Int32 k_albedoTextureSlot = 0;
constexpr Int32 k_aoTextureSlot     = 1;
constexpr Int32 k_normalTextureSlot = 2;

struct CBTextureMappingSceneEntity
{
    static constexpr Int32 k_slot = 0;

    Mtx4   cb_world         = Numbers<Mtx4>::k_identity;
    Mtx4   cb_worldInvTrans = Numbers<Mtx4>::k_identity;
    UInt32 cb_flags         = 0;
    float  padding[3]       = {};
};

struct CBTextureMappingSceneCamera
{
    static constexpr Int32 k_slot = 1;

    Vec3  cb_eyePos   = Numbers<Vec3>::k_zero;
    float padding1    = 0.f;
    Mtx4  cb_viewProj = Numbers<Mtx4>::k_identity;
};

struct CBTextureMappingSceneLight
{
    static constexpr Int32 k_slot = 2;

    Vec3  cb_lightDir     = { 0.f, -1.f, 0.f };
    float padding2        = 0.f;
    Vec3  cb_lightColor   = Numbers<Vec3>::k_one;
    float padding3        = 0.f;
    Vec3  cb_ambientColor = Numbers<Vec3>::k_one;
    float padding4        = 0.f;
};

struct TextureMappingSceneVertex
{
    Vec3 pos      = Numbers<Vec3>::k_zero;
    Vec2 texcoord = Numbers<Vec2>::k_zero;
    Vec3 normal   = Numbers<Vec3>::k_zero;
    Vec3 tangent  = Numbers<Vec3>::k_zero;

    inline static VertexLayout s_layout = [] {
        VertexLayout layout = {};
        layout.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        layout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
        layout.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);
        return layout;
    }();
};

}   // namespace

TextureMappingScene::TextureMappingScene()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    vsInfo.entryPoint         = "VSMain";
    vsInfo.stage              = eShader::VertexShader;
    const ShaderCompiler vsc  = ShaderCompiler::CompileHLSLFile("Resources/TextureMappingScene/TextureMappingSceneShader.hlsl", vsInfo);
    JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
    const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

    ShaderCompilerInfo psInfo = {};
    psInfo.entryPoint         = "PSMain";
    psInfo.stage              = eShader::PixelShader;
    const ShaderCompiler psc  = ShaderCompiler::CompileHLSLFile("Resources/TextureMappingScene/TextureMappingSceneShader.hlsl", psInfo);
    JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
    const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

    m_ph = gfx.CreateProgram(vsh, psh, true);

    m_entityCbh = gfx.CreateConstantBuffer(sizeof(CBTextureMappingSceneEntity));
    m_cmrCbh    = gfx.CreateConstantBuffer(sizeof(CBTextureMappingSceneCamera));
    m_lightCbh  = gfx.CreateConstantBuffer(sizeof(CBTextureMappingSceneLight));

    // load model
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/TextureMappingScene/Model.obj", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModel = importer->ConvertToModel(TextureMappingSceneVertex::s_layout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModel, "Failed to extract model: {}", pModel.GetError().ToErrorMessage());
    m_pMesh = pModel.GetValue()->GetMeshes()[0];

    // load texture
    m_pAlbedo = Texture::LoadFromFile("Resources/TextureMappingScene/Albedo.png").GetValue();
    m_pAo     = Texture::LoadFromFile("Resources/TextureMappingScene/Ao.png").GetValue();
    m_pNormal = Texture::LoadFromFile("Resources/TextureMappingScene/Normal.png").GetValue();

    // camera setting
    m_camera.SetAspectRatio(16.f / 9.f);
    m_cameraTransform.SetTranslationZ(-8.f);
}

TextureMappingScene::~TextureMappingScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
}

void TextureMappingScene::OnUpdate(
    const float _deltaTime)
{
    if (m_bAnimate)
    {
        m_entityYRad += m_dMoveRad * _deltaTime;
        m_entityYawRad += m_dYawRad * _deltaTime;
    }
}

void TextureMappingScene::OnRender(
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
    const Mtx4                  vpMtx    = m_cameraTransform.GetMatrix().Invert() * m_camera.GetProjMatrix();
    CBTextureMappingSceneCamera cbCamera = {};
    cbCamera.cb_eyePos                   = m_cameraTransform.GetTranslation();
    cbCamera.cb_viewProj                 = vpMtx.Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBTextureMappingSceneCamera::k_slot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBTextureMappingSceneCamera::k_slot);

    // light
    CBTextureMappingSceneLight cbLight = {};
    cbLight.cb_lightDir                = Normalized(m_lightLookAt - m_lightPos);
    cbLight.cb_lightColor              = m_lightColor;
    cbLight.cb_ambientColor            = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBTextureMappingSceneLight::k_slot);

    // textures & sampler
    m_pAlbedo->Bind(eShader::PixelShader, k_albedoTextureSlot);
    m_pAo->Bind(eShader::PixelShader, k_aoTextureSlot);
    m_pNormal->Bind(eShader::PixelShader, k_normalTextureSlot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBTextureMappingSceneEntity::k_slot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBTextureMappingSceneEntity::k_slot);

    CBTextureMappingSceneEntity cbEntity      = {};
    constexpr Int32             k_numEntities = 1;
    for (Int32 i = 0; i < k_numEntities; ++i)
    {
        float t   = static_cast<float>(i) * 15.f;
        float yaw = m_entityYawRad + ToRad(t);
        float x   = static_cast<float>(i - k_numEntities / 2);

        const Mtx4 world          = Mtx4::CreateFromYawPitchRoll(yaw, 0.f, 0.f) * Mtx4::CreateTranslation(x, 0.f, 0.f);
        cbEntity.cb_world         = world.Transpose();
        cbEntity.cb_worldInvTrans = world.Invert();
        cbEntity.cb_flags         = m_textureMappingFlags.GetValue();
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
        m_pMesh->Submit();
    }

    // blit inner framebuffer texture to output texture
    // output framebuffer maybe do not have depth texture.
    // so. draw on screen-off framebuffer and blit the color texture to output framebuffer.
    // in real case. use    post-process pass to do this.
    const FrameBufferInfo fbInfo = gfx.GetFrameBufferInfo(_view.fbh);
    JAM_ASSERT(fbInfo.numRenderTargets > 0, "Back buffer must have at least one render target");
    gfx.CopyTexture(fbInfo.renderTargetTexhs[0], 0, 0, 0, 0, 0, m_fbTexh, 0, 0, 0, 0, 0);
}

void TextureMappingScene::OnImGuiRender()
{
    ImGui::Begin("TextureMappingScene Controller");

    ImGui::Text("FPS: %.2f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Entity Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Rotation", &m_bAnimate);
    }

    bool bAlbedo = m_textureMappingFlags.IsEnabled(eTextureMappingFlags::Albedo);
    if (ImGui::Checkbox("Albedo Mapping", &bAlbedo))
    {
        m_textureMappingFlags.Set(eTextureMappingFlags::Albedo, bAlbedo);
    }

    bool bAo = m_textureMappingFlags.IsEnabled(eTextureMappingFlags::Ao);
    if (ImGui::Checkbox("AO Mapping", &bAo))
    {
        m_textureMappingFlags.Set(eTextureMappingFlags::Ao, bAo);
    }

    bool bNormal = m_textureMappingFlags.IsEnabled(eTextureMappingFlags::Normal);
    if (ImGui::Checkbox("Normal Mapping", &bNormal))
    {
        m_textureMappingFlags.Set(eTextureMappingFlags::Normal, bNormal);
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

void TextureMappingScene::OnResize(
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
