#include "pch.h"
#include "PBRScene.h"
#include "../JamEngine/AssimpImporter.h"

namespace
{

constexpr Int32 k_pbrSceneLinearWrapSlot  = 0;
constexpr Int32 k_pbrSceneLinearClampSlot = 1;

constexpr Int32 k_pbrSceneHdrSceneSlot            = 0;
constexpr Int32 k_pbrSceneIblIrradianceSlot       = 0;
constexpr Int32 k_pbrSceneIblSpecularSlot         = 1;
constexpr Int32 k_pbrSceneIblEnvironmentSlot      = 2;
constexpr Int32 k_pbrSceneBrdfLUTSlot             = 2;
constexpr Int32 k_pbrSceneAlbedoSlot              = 3;
constexpr Int32 k_pbrSceneAoRoughnessMetallicSlot = 4;
constexpr Int32 k_pbrSceneEmissiveSlot            = 5;
constexpr Int32 k_pbrSceneNormalSlot              = 6;

struct CBPBRSceneEntity
{
    static constexpr Int32 k_slot = 0;

    Mtx4 cb_world         = Numbers<Mtx4>::k_identity;
    Mtx4 cb_worldInvTrans = Numbers<Mtx4>::k_identity;
};

struct CBPBRSceneCamera
{
    static constexpr Int32 k_slot = 1;

    Vec3  cb_eyePos   = Numbers<Vec3>::k_zero;
    float padding1    = 0.f;
    Mtx4  cb_viewProj = Numbers<Mtx4>::k_identity;
};

struct CBPBRSceneLight
{
    static constexpr Int32 k_slot = 2;

    struct Light
    {
        Vec3  lightPos = Numbers<Vec3>::k_zero;
        float padding  = 0.f;

        Vec3  lightDir     = Numbers<Vec3>::k_zero;
        float fallOffStart = 5.f;

        Vec3  lightColor = Numbers<Vec3>::k_one;
        float fallOffEnd = 10.f;

        float innerConeCos = 0.f;
        float outerConeCos = 0.f;
        Int32 bEnabled     = 0;
        Int32 type         = 0;   // 0: directional, 1: point, 2: spot
    };

    Light cb_lights[4] = {};
};

struct CBPBRSceneCubeMap
{
    constexpr static Int32 k_slot = 2;

    UInt32 cb_type    = 0;   // 0: none, 1: irradiance, 2: specular, 3: environment
    float  padding[3] = {};
};

struct PBRSceneVertex
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

PBRScene::PBRScene()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo vsInfo = {};
    ShaderCompilerInfo psInfo = {};

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
    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile("Resources/PBRScene/scene.gltf", eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::GenerateNormals | eAssimpImporterFlags::GenerateTangentFrame);
    JAM_ASSERT(importer, "Failed to load model: {}", importer.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModel = importer->ConvertToModel(PBRSceneVertex::s_layout, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModel, "Failed to extract model: {}", pModel.GetError().ToErrorMessage());
    m_pMesh = pModel.GetValue()->GetMeshes()[0];

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
    vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);

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
    if (m_bAnimate)
    {
        m_entityYawRad += m_dYawRad * _deltaTime;
    }

    m_camera.Update(_deltaTime);
}

void PBRScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();
    gfx.SetFrameBuffer(m_fbh);
    gfx.ClearRenderTarget(m_fbh, RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f));
    gfx.ClearDepthStencil(m_fbh, true, false, 1, 0);
    gfx.SetViewport(m_vp);
    gfx.SetBlendFlags(eBlendFlags::None | eBlendFlags::WriteMask_All, 0);

    // camera
    const Mtx4       vpMtx    = m_camera.trans.GetMatrix().Invert() * m_camera.cmr.GetProjMatrix();
    CBPBRSceneCamera cbCamera = {};
    cbCamera.cb_eyePos        = m_camera.trans.GetTranslation();
    cbCamera.cb_viewProj      = vpMtx.Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(cbCamera));
    gfx.SetConstantBuffer(m_cmrCbh, eShader::VertexShader, CBPBRSceneCamera::k_slot);
    gfx.SetConstantBuffer(m_cmrCbh, eShader::PixelShader, CBPBRSceneCamera::k_slot);

    // cubemap
    CBPBRSceneCubeMap cbCubeMap = {};
    cbCubeMap.cb_type           = UnderlyingFrom(m_cubeMapType);
    gfx.UpdateBuffer(m_cubeMapCbh, AsMemoryView(cbCubeMap));
    gfx.SetConstantBuffer(m_cubeMapCbh, eShader::PixelShader, CBPBRSceneCubeMap::k_slot);
    gfx.SetVertexBuffer(m_cubeVbh);
    gfx.SetIndexBuffer(m_cubeIbh);
    gfx.SetProgram(m_cubeMapPh);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_LessEqual | eRenderFlags::DepthWrite | eRenderFlags::CCWFront);
    m_pIBLIrradiance->Bind(eShader::PixelShader, k_pbrSceneIblIrradianceSlot);
    m_pIBLSpecular->Bind(eShader::PixelShader, k_pbrSceneIblSpecularSlot);
    m_pIBLEnvironment->Bind(eShader::PixelShader, k_pbrSceneIblEnvironmentSlot);
    gfx.SetSampler(eSamplerFlags::AddressU_Wrap | eSamplerFlags::AddressV_Wrap | eSamplerFlags::AddressW_Wrap | eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear, eShader::PixelShader, k_pbrSceneLinearWrapSlot);
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

        CBPBRSceneLight::Light& cb = cbLight.cb_lights[i];
        cb.lightPos                = l.pos;
        cb.lightDir                = Normalized(l.lookAt - l.pos);
        cb.lightColor              = l.color;
        cb.fallOffStart            = l.fallOffStart;
        cb.fallOffEnd              = l.fallOffEnd;
        cb.innerConeCos            = Cos(l.innerConeRad);
        cb.outerConeCos            = Cos(l.outerConeRad);
        cb.bEnabled                = l.bEnabled ? 1 : 0;
        cb.type                    = UnderlyingFrom(l.type);
    }

    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(cbLight));
    gfx.SetConstantBuffer(m_lightCbh, eShader::PixelShader, CBPBRSceneLight::k_slot);

    // textures & sampler
    m_pAlbedo->Bind(eShader::PixelShader, k_pbrSceneAlbedoSlot);
    m_pNormal->Bind(eShader::PixelShader, k_pbrSceneNormalSlot);
    m_pAoRoughnessMetallic->Bind(eShader::PixelShader, k_pbrSceneAoRoughnessMetallicSlot);
    m_pEmissive->Bind(eShader::PixelShader, k_pbrSceneEmissiveSlot);
    m_pBRDFLUT->Bind(eShader::PixelShader, k_pbrSceneBrdfLUTSlot);
    gfx.SetSampler(eSamplerFlags::AddressU_Clamp | eSamplerFlags::AddressV_Clamp | eSamplerFlags::AddressW_Clamp | eSamplerFlags::Filter_MinLinear_MagLinear_MipLinear, eShader::PixelShader, k_pbrSceneLinearClampSlot);

    // entity
    gfx.SetProgram(m_ph);
    gfx.SetRenderFlags(eRenderFlags::DepthFunc_Less | eRenderFlags::DepthWrite | eRenderFlags::CullMode_Back);

    gfx.SetConstantBuffer(m_entityCbh, eShader::VertexShader, CBPBRSceneEntity::k_slot);
    gfx.SetConstantBuffer(m_entityCbh, eShader::PixelShader, CBPBRSceneEntity::k_slot);

    CBPBRSceneEntity cbEntity = {};
    const Mtx4       world    = Mtx4::CreateFromYawPitchRoll(m_entityYawRad, ToRad(-90.f), 0.f);
    cbEntity.cb_world         = world.Transpose();
    cbEntity.cb_worldInvTrans = world.Invert();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cbEntity));
    m_pMesh->Submit();

    gfx.SetFrameBuffer(k_invalidHandle);
    gfx.SetProgram(m_toneMappingPh);
    gfx.SetVertexBuffer(m_fullTriVbh);
    gfx.SetIndexBuffer(m_fullTriIbh);
    gfx.SetTexture(m_fbTexh, eShader::PixelShader, k_pbrSceneHdrSceneSlot);
    gfx.Submit();
    gfx.SetTexture(k_invalidHandle, eShader::PixelShader, k_pbrSceneHdrSceneSlot);
    gfx.Touch();
}

void PBRScene::OnImGuiRender()
{
    ImGui::Begin("PBRScene Controller");

    // 1. Performance
    ImGui::Text("FPS: %.2f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();

    // 2. Entity & Animation
    if (ImGui::CollapsingHeader("Entity & Animation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Animate Rotation", &m_bAnimate);

        float yawDeg = ToDeg(m_dYawRad);
        if (ImGui::SliderFloat("Yaw Speed (Deg/s)", &yawDeg, 0.f, 360.f))
        {
            m_dYawRad = ToRad(yawDeg);
        }
    }

    // 4. Lights (Array<Light, 4> 제어)
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (Int32 i = 0; i < static_cast<Int32>(m_lights.size()); ++i)
        {
            ImGui::PushID(i);   // ID 충돌 방지

            char label[32];
            sprintf_s(label, "Light %d", i);

            if (ImGui::TreeNode(label))
            {
                Light& l = m_lights[i];

                ImGui::Checkbox("Enabled", &l.bEnabled);

                // Light Type 선택
                const char* lightTypes[] = { "Directional", "Point", "Spot" };
                Int32       currentType  = UnderlyingFrom(l.type);
                if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
                {
                    l.type = static_cast<eLight>(currentType);
                }

                // 공통 속성: Color (HDR 고려하여 DragFloat3 또는 ColorEdit3)
                ImGui::ColorEdit3("Color", &l.color.x, ImGuiColorEditFlags_Float);

                if (l.type != eLight::Directional)
                {
                    ImGui::DragFloat3("Position", &l.pos.x, 0.1f);
                    ImGui::DragFloat("Falloff Start", &l.fallOffStart, 0.1f, 0.f, l.fallOffEnd);
                    ImGui::DragFloat("Falloff End", &l.fallOffEnd, 0.1f, l.fallOffStart, 100.f);
                }

                if (l.type != eLight::Point)
                {
                    ImGui::DragFloat3("Look At", &l.lookAt.x, 0.1f);
                }

                if (l.type == eLight::Spot)
                {
                    float innerDeg = ToDeg(l.innerConeRad);
                    float outerDeg = ToDeg(l.outerConeRad);

                    if (ImGui::SliderFloat("Inner Cone (Deg)", &innerDeg, 0.f, outerDeg))
                    {
                        l.innerConeRad = ToRad(innerDeg);
                    }
                    if (ImGui::SliderFloat("Outer Cone (Deg)", &outerDeg, innerDeg, 179.f))
                    {
                        l.outerConeRad = ToRad(outerDeg);
                    }
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
            if (i < static_cast<Int32>(m_lights.size()) - 1) ImGui::Separator();
        }
    }

    ImGui::End();
}

void PBRScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    JAM_DESTROY_FRAME_BUFFER(m_fbh);

    Graphics& gfx = GetGraphics();
    m_fbh         = gfx.CreateFrameBuffer(_width, _height, eTextureFormat::RGBA16_Float, eTextureFormat::D24_UNorm_S8_UInt);
    m_vp          = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };

    const FrameBufferInfo info = gfx.GetFrameBufferInfo(m_fbh);
    JAM_ASSERT(info.numRenderTargets > 0, "Frame buffer must have at least one render target");
    m_fbTexh = info.renderTargetTexhs[0];
}
