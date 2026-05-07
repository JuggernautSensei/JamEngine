#include "pch.h"
#include "PBRScene.h"

#include "Gui.h"
#include "RenderStates.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

constexpr Int32 kPbrSceneLinearClampSlot = 0;
constexpr Int32 kPbrSceneLinearWrapSlot  = 1;

constexpr Int32 kPbrSceneHdrSceneSlot            = 0;
constexpr Int32 kPbrSceneIblIrradianceSlot       = 0;
constexpr Int32 kPbrSceneIblSpecularSlot         = 1;
constexpr Int32 kPbrSceneIblEnvironmentSlot      = 2;
constexpr Int32 kPbrSceneBrdfLUTSlot             = 2;
constexpr Int32 kPbrSceneAlbedoSlot              = 3;
constexpr Int32 kPbrSceneAoRoughnessMetallicSlot = 4;
constexpr Int32 kPbrSceneEmissiveSlot            = 5;
constexpr Int32 kPbrSceneNormalSlot              = 6;

struct CBPBRSceneEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4 world;
    Mtx4 invTransWorld;
};

struct CBPBRSceneCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
    float padding1;
    Mtx4  viewProj;
};

struct CBPBRSceneLight
{
    static constexpr Int32 kSlot = 2;

    struct Light
    {
        Vec3  lightPos;
        float padding;

        Vec3  lightDir;
        float fallOffStart;

        Vec3  lightColor;
        float fallOffEnd;

        float innerConeCos;
        float outerConeCos;
        Int32 bEnabled;
        Int32 type;   // 0: directional, 1: point, 2: spot
    };

    Light lights[4];
};

struct CBPBRSceneCubeMap
{
    constexpr static Int32 kSlot = 2;

    UInt32 type;   // 0: none, 1: irradiance, 2: specular, 3: environment
    float  padding[3];
};

}   // namespace

PBRScene::PBRScene()
{
    Graphics& gfx = GetGraphics();

    ShaderLoadInfo vsInfo = {};
    ShaderLoadInfo psInfo = {};

    // main shader
    {
        vsInfo.entryPoint        = "VSMain";
        vsInfo.stage             = eShader::VertexShader;
        const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneShader.hlsl", vsInfo);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        psInfo.entryPoint        = "PSMain";
        psInfo.stage             = eShader::PixelShader;
        const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneShader.hlsl", psInfo);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_ph = gfx.CreateProgram(vsh, psh, true);
    }

    // tone mapping
    {
        const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneToneMappingShader.hlsl", vsInfo);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneToneMappingShader.hlsl", psInfo);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_toneMappingPh = gfx.CreateProgram(vsh, psh, true);
    }

    // cube map
    {
        const ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneCubeMapShader.hlsl", vsInfo);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        const ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        const ShaderCompiler psc = ShaderCompiler::CompileHLSLFile("Resources/PBRScene/PBRSceneCubeMapShader.hlsl", psInfo);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        const ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_cubeMapPh = gfx.CreateProgram(vsh, psh, true);
    }

    // constant buffer
    m_entityCbh  = gfx.CreateConstantBuffer(sizeof(CBPBRSceneEntity));
    m_cmrCbh     = gfx.CreateConstantBuffer(sizeof(CBPBRSceneCamera));
    m_lightCbh   = gfx.CreateConstantBuffer(sizeof(CBPBRSceneLight));
    m_cubeMapCbh = gfx.CreateConstantBuffer(sizeof(CBPBRSceneCubeMap));

    // load model
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/PBRScene/scene.gltf", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModelOrNot = importer->ConvertToModel(attribLayout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModelOrNot, "Failed to extract model: {}", pModelOrNot.GetError().ToErrorMessage());
    m_pMesh = pModelOrNot.GetValue()->GetMeshes()[0];

    // load texture
    m_pAlbedo              = Texture::LoadFromFile("Resources/PBRScene/Albedo.jpeg").GetValue();
    m_pAoRoughnessMetallic = Texture::LoadFromFile("Resources/PBRScene/AoRoughnessMetaliic.png").GetValue();
    m_pEmissive            = Texture::LoadFromFile("Resources/PBRScene/Emissive.jpeg").GetValue();
    m_pNormal              = Texture::LoadFromFile("Resources/PBRScene/Normal.jpeg").GetValue();
    m_pIBLIrradiance       = Texture::LoadFromFile("Resources/PBRScene/IBLDiffuse.dds").GetValue();
    m_pIBLSpecular         = Texture::LoadFromFile("Resources/PBRScene/IBLSpecular.dds").GetValue();
    m_pIBLEnvironment      = Texture::LoadFromFile("Resources/PBRScene/IBLEnv.dds").GetValue();
    m_pBRDFLUT             = Texture::LoadFromFile("Resources/PBRScene/IBLBrdf.dds").GetValue();

    // tonemapping triangle
    Vec3 triVertices[] = {
        { -1.f,  1.f, 0.f },
        {  3.f,  1.f, 0.f },
        { -1.f, -3.f, 0.f }
    };

    UInt16 triIndices[] = {
        0, 1, 2
    };

    VertexLayout vl = {};
    vl.Add(eVertexAttribute::Position, eAttributeFormat::Float3);

    m_fullTriVbh = gfx.CreateVertexBuffer(AsMemoryView(triVertices), vl);
    m_fullTriIbh = gfx.CreateIndexBuffer(AsMemoryView(triIndices));

    // cube map
    Vec3 cubemap[8] = {
        { -1.f,  1.f, -1.f },
        {  1.f,  1.f, -1.f },
        {  1.f, -1.f, -1.f },
        { -1.f, -1.f, -1.f },
        { -1.f,  1.f,  1.f },
        {  1.f,  1.f,  1.f },
        {  1.f, -1.f,  1.f },
        { -1.f, -1.f,  1.f }
    };

    UInt16 cubeIndices[] = {
        0, 1, 2, 0, 2, 3, 1, 5, 6, 1, 6, 2, 5, 4, 7, 5, 7, 6, 4, 0, 3, 4, 3, 7, 4, 5, 1, 4, 1, 0, 3, 2, 6, 3, 6, 7
    };

    m_cubeVbh = gfx.CreateVertexBuffer(AsMemoryView(cubemap), vl);
    m_cubeIbh = gfx.CreateIndexBuffer(AsMemoryView(cubeIndices));

    // camera setting
    m_camera.cmr.SetAspectRatio(16.f / 9.f);
    m_camera.trans.SetTranslationZ(-5.f);

    m_lights[0].bEnabled   = true;
    m_lights[0].type       = eLight::Point;
    m_lights[0].pos        = Vec3(2.f, 2.f, -2.f);
    m_lights[0].fallOffEnd = 50.f;
}

PBRScene::~PBRScene()
{
    JAM_DESTROY_PROGRAM(m_ph);
    JAM_DESTROY_PROGRAM(m_toneMappingPh);
    JAM_DESTROY_PROGRAM(m_cubeMapPh);
    JAM_DESTROY_CONSTANT_BUFFER(m_entityCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cmrCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_lightCbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cubeMapCbh);
    JAM_DESTROY_FRAME_BUFFER(m_fbh);
    JAM_DESTROY_VERTEX_BUFFER(m_fullTriVbh);
    JAM_DESTROY_INDEX_BUFFER(m_fullTriIbh);
    JAM_DESTROY_VERTEX_BUFFER(m_cubeVbh);
    JAM_DESTROY_INDEX_BUFFER(m_cubeIbh);
}

void PBRScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_rot += m_dRot * _deltaTime;
    }
}

void PBRScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(_view.vp);
    gfx.SetBlendFlags(kOpaqueBlend, 0);

    // camera
    CBPBRSceneCamera cbCamera = {};
    cbCamera.eyePos           = m_camera.GetEyePos();
    cbCamera.viewProj         = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBPBRSceneCamera::kSlot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBPBRSceneCamera::kSlot);

    // cubemap
    CBPBRSceneCubeMap cbCubeMap = {};
    cbCubeMap.type              = UnderlyingFrom(m_cubeMapType);
    gfx.UpdateBuffer(m_cubeMapCbh, AsMemoryView(cbCubeMap));
    gfx.SetConstantBuffer(m_cubeMapCbh, eShader::PixelShader, CBPBRSceneCubeMap::kSlot);
    gfx.SetVertexBuffer(m_cubeVbh);
    gfx.SetIndexBuffer(m_cubeIbh);
    gfx.SetProgram(m_cubeMapPh);
    gfx.SetRenderFlags(kSkyboxRenderFlags);
    m_pIBLIrradiance->Bind(eShader::PixelShader, kPbrSceneIblIrradianceSlot);
    m_pIBLSpecular->Bind(eShader::PixelShader, kPbrSceneIblSpecularSlot);
    m_pIBLEnvironment->Bind(eShader::PixelShader, kPbrSceneIblEnvironmentSlot);
    gfx.SetSampler(kLinearWrap, eShader::PixelShader, kPbrSceneLinearWrapSlot);
    gfx.Submit();

    // light
    CBPBRSceneLight cbLight = {};
    for (Int32 i = 0; i < m_lights.size(); ++i)
    {
        const Light& l = m_lights[i];
        if (!l.bEnabled)
        {
            continue;
        }

        CBPBRSceneLight::Light& cb = cbLight.lights[i];
        cb.lightPos                = l.pos;
        cb.lightDir                = Vec3 { l.lookAt - l.pos }.Normalized();
        cb.lightColor              = l.color;
        cb.fallOffStart            = l.fallOffStart;
        cb.fallOffEnd              = l.fallOffEnd;
        cb.innerConeCos            = Cos(l.innerConeRad);
        cb.outerConeCos            = Cos(l.outerConeRad);
        cb.bEnabled                = l.bEnabled ? 1 : 0;
        cb.type                    = UnderlyingFrom(l.type);
    }

    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBPBRSceneLight::kSlot);

    // textures & sampler
    m_pAlbedo->Bind(eShader::PixelShader, kPbrSceneAlbedoSlot);
    m_pNormal->Bind(eShader::PixelShader, kPbrSceneNormalSlot);
    m_pAoRoughnessMetallic->Bind(eShader::PixelShader, kPbrSceneAoRoughnessMetallicSlot);
    m_pEmissive->Bind(eShader::PixelShader, kPbrSceneEmissiveSlot);
    m_pBRDFLUT->Bind(eShader::PixelShader, kPbrSceneBrdfLUTSlot);
    gfx.SetSampler(kLinearClamp, eShader::PixelShader, kPbrSceneLinearClampSlot);

    // entity
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(kBackCullDepthTest);
    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBPBRSceneEntity::kSlot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBPBRSceneEntity::kSlot);

    CBPBRSceneEntity cbEntity = {};
    cbEntity.world            = Mtx4::FromEuler({ ToRad(-90.f), m_rot, 0.f }).Transpose();
    cbEntity.invTransWorld    = cbEntity.world;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pMesh->Submit();

    gfx.SetFrameBuffer(kInvalidHandle);
    gfx.SetProgram(m_toneMappingPh);
    gfx.SetVertexBuffer(m_fullTriVbh);
    gfx.SetIndexBuffer(m_fullTriIbh);
    gfx.SetTexture(m_colorTexh, eShader::PixelShader, kPbrSceneHdrSceneSlot);
    gfx.Submit();
    gfx.SetTexture(kInvalidHandle, eShader::PixelShader, kPbrSceneHdrSceneSlot);
    gfx.Touch();
}

void PBRScene::OnImGuiRender()
{
    if (!BeginSceneInspectorWindows("PBRScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::End();
}

void PBRScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    Graphics& gfx = GetGraphics();

    const TextureHandle depthTexh = gfx.CreateTexture2D(_width, _height, eTextureFormat::D32_Float, false, 1, eTextureCreateFlags::DepthStencilAttachment);
    const TextureHandle colorTexh = gfx.CreateTexture2D(_width, _height, eTextureFormat::RGBA16_Float, false, 1, eTextureCreateFlags::RenderTargetAttachment);

    Attachment attachment[2];
    attachment[0].texh = colorTexh;
    attachment[1].texh = depthTexh;

    m_fbh       = gfx.CreateFrameBuffer(attachment, true);
    m_colorTexh = colorTexh;

    m_camera.cmr.SetAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
}
