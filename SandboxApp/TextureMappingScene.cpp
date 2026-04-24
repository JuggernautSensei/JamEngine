#include "pch.h"
#include "TextureMappingScene.h"

#include "Gui.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

constexpr Int32 kLinearSamplerSlot = 0;
constexpr Int32 kAlbedoTextureSlot = 0;
constexpr Int32 kAoTextureSlot     = 1;
constexpr Int32 kNormalTextureSlot = 2;

struct CBTextureMappingSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    Mtx4   worldInvTrans;
    UInt32 flags;
    float  padding[3];
};

struct CBTextureMappingSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding1;
    Mtx4  viewProj;
};

struct CBTextureMappingSceneLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightDir;
    float padding2;
    Vec3  lightColor;
    float padding3;
    Vec3  ambientColor;
    float padding4;
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
    VertexLayout attribLayout = {};
    attribLayout.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/TextureMappingScene/Model.obj", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModel = importer->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModel, "Failed to extract model: {}", pModel.GetError().ToErrorMessage());
    m_pMesh = pModel.GetValue()->GetMeshes()[0];

    // load texture
    m_pAlbedo = Texture::LoadFromFile("Resources/TextureMappingScene/Albedo.png").GetValue();
    m_pAo     = Texture::LoadFromFile("Resources/TextureMappingScene/Ao.png").GetValue();
    m_pNormal = Texture::LoadFromFile("Resources/TextureMappingScene/Normal.png").GetValue();

    // camera setting
    m_camera.trans.SetTranslationZ(-10.f);
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
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_rot += m_dRot * _deltaTime;
    }
}

void TextureMappingScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(_view.vp);
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    CBTextureMappingSceneCamera cbCamera = {};
    cbCamera.eyePos                      = m_camera.GetEyePos();
    cbCamera.viewProj                    = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBTextureMappingSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBTextureMappingSceneCamera::kSlot);

    // light
    CBTextureMappingSceneLight cbLight = {};
    cbLight.lightDir                   = Vec3 { m_lightLookAt - m_lightPos }.Normalized();
    cbLight.lightColor                 = m_lightColor;
    cbLight.ambientColor               = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBTextureMappingSceneLight::kSlot);

    // textures & sampler
    m_pAlbedo->Bind(eShader::PixelShader, kAlbedoTextureSlot);
    m_pAo->Bind(eShader::PixelShader, kAoTextureSlot);
    m_pNormal->Bind(eShader::PixelShader, kNormalTextureSlot);
    gfx.SetSampler(eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear | eSamplerFlags::AddressU_Wrap | eSamplerFlags::AddressV_Wrap, eShader::PixelShader, kLinearSamplerSlot);

    // entity
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBTextureMappingSceneEntity::kSlot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBTextureMappingSceneEntity::kSlot);

    CBTextureMappingSceneEntity cbEntity = {};
    const Vec3                  euler    = { m_rot, m_rot, 0.f };
    cbEntity.world                       = Mtx4::CreateSRT(Vec3::kOne, euler, Vec3::kZero).Transpose();
    cbEntity.worldInvTrans               = Mtx4::CreateInvSRT(Vec3::kOne, euler, Vec3::kZero);
    cbEntity.flags                       = m_textureMappingFlags.GetValue();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pMesh->Submit();

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
    if (!BeginSceneInspectorWindows("TextureMappingScene"))
    {
        ImGui::End();
        return;
    }

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
        ImGui::DragFloat3("Light Pos", &m_lightPos.x, 0.1f);
        ImGui::DragFloat3("Light LookAt", &m_lightLookAt.x, 0.1f);
        ImGui::ColorEdit3("Light Color", &m_lightColor.x);
        ImGui::ColorEdit3("Ambient Color", &m_ambientColor.x);
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
    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
