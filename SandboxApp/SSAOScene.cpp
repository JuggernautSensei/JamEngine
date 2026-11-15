#include "pch.h"
#include "SSAOScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RandomGenerator.h"
#include "Utilities.h"
#include "RenderStates.h"
#include "../JamEngine/ModelImporter.h"

#define SCENE_PATH(_filepath) "Resources/SSAOScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec3  baseColor;
    float padding1;
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4 view;
    Mtx4 viewProj;
};

struct CBSsao
{
    static constexpr Int32 kSlot = 0;

    Mtx4  proj;
    Mtx4  invProj;
    Vec4  kernel[64];
    Vec2  screenSize;
    float radius;
    float bias;
    float power;
    float padding[3];
};

struct CBComposite
{
    static constexpr Int32 kSlot = 0;

    Vec3  lightDirV;
    float ambient;
    float bUseAo;
    float bAoOnly;
    float padding[2];
};

constexpr Int32 kNoiseSize = 4;

constexpr Flags<eRenderFlags> kNoCullDepthTest =
    eRenderFlags::DepthFunc_Less
    | eRenderFlags::DepthWrite;

constexpr Vec3 kLightDirW = { 0.3f, 0.8f, 0.4f };

}   // namespace

SSAOScene::SSAOScene()
{
    InitModel_();
    InitKernel_();
    InitRenderGraph_();

    m_camera.camera.SetNearZ(0.05f);
    m_camera.camera.SetFarZ(100.f);
    m_camera.dMove = 5.f;
    m_camera.transform.SetTranslation({ -9.f, 2.f, 0.f });
    m_camera.euler.yaw = ToRad(90.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

SSAOScene::~SSAOScene()
{
    JAM_DESTROY_TEXTURE(m_noiseTexh);
}

void SSAOScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);
}

void SSAOScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.view     = m_camera.GetView().Transpose();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(camera));

    const Vec2 screenSize = { static_cast<float>(_view.viewport.width), static_cast<float>(_view.viewport.height) };

    CBSsao ssao     = {};
    ssao.proj       = m_camera.GetProj().Transpose();
    ssao.invProj    = m_camera.GetInvProj().Transpose();
    ssao.screenSize = screenSize;
    ssao.radius     = m_radius;
    ssao.bias       = m_bias;
    ssao.power      = m_power;
    for (Int32 i = 0; i < kKernelSize; ++i)
    {
        ssao.kernel[i] = m_kernel[i];
    }
    gfx.UpdateBuffer(m_ssaoCbh, AsMemoryView(ssao));

    CBComposite composite = {};
    composite.lightDirV   = m_camera.GetView().MultVector(kLightDirW.Normalized()).Normalized();
    composite.ambient     = m_ambient;
    composite.bUseAo      = m_bUseAo ? 1.f : 0.f;
    composite.bAoOnly     = m_bAoOnly ? 1.f : 0.f;
    gfx.UpdateBuffer(m_compositeCbh, AsMemoryView(composite));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void SSAOScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("SSAOScene"))
    {
        ImGui::End();
        return;
    }

    // ssao
    ImGui::SeparatorText("SSAO");
    ImGui::Checkbox("Enable SSAO", &m_bUseAo);
    ImGui::Checkbox("Show AO Only", &m_bAoOnly);
    ImGui::SliderFloat("Radius", &m_radius, 0.05f, 2.f);
    ImGui::SliderFloat("Bias", &m_bias, 0.f, 0.1f);
    ImGui::SliderFloat("Power", &m_power, 0.5f, 4.f);

    // light
    ImGui::SeparatorText("Light");
    ImGui::SliderFloat("Ambient", &m_ambient, 0.1f, 0.9f);

    ImGui::End();
}

void SSAOScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void SSAOScene::InitModel_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    const Flags<eAssimpImporterFlags> flags = eAssimpImporterFlags::LoadMeshes | eAssimpImporterFlags::LoadMaterials | eAssimpImporterFlags::LoadTextures;

    Expect<AssimpImporter> importerOrNot = AssimpImporter::ImportFromFile(SCENE_PATH("Sponza.glb"), flags);
    JAM_ASSERT(importerOrNot, "Failed to import Sponza. {}", importerOrNot.GetError());

    Expect<Ref<Model>> modelOrNot = importerOrNot->ConvertToModel(layout, eAssimpConvertToModelFlags::All);
    JAM_ASSERT(modelOrNot, "Failed to convert Sponza to model. {}", modelOrNot.GetError());
    m_pModel = modelOrNot.GetValue();
}

void SSAOScene::InitKernel_()
{
    Graphics& gfx = GetGraphics();

    // 반구 커널. 중심에 가까운 샘플이 더 많도록 길이를 제곱으로 분포
    for (Int32 i = 0; i < kKernelSize; ++i)
    {
        Vec3 sample = Vec3 { MakeRandomFloat(-1.f, 1.f), MakeRandomFloat(-1.f, 1.f), MakeRandomFloat(0.f, 1.f) }.Normalized();
        sample *= MakeRandomFloat(0.f, 1.f);

        const float t = static_cast<float>(i) / static_cast<float>(kKernelSize);
        sample *= Lerp(0.1f, 1.f, t * t);
        m_kernel[i] = Vec4 { sample.x, sample.y, sample.z, 0.f };
    }

    // 커널 회전용 노이즈
    Array<Vec4, kNoiseSize * kNoiseSize> noise = {};
    for (Vec4& v: noise)
    {
        v = Vec4 { MakeRandomFloat(-1.f, 1.f), MakeRandomFloat(-1.f, 1.f), 0.f, 0.f };
    }

    const MemoryView initData[] = { AsMemoryView(noise) };
    m_noiseTexh                 = gfx.CreateTexture2D(kNoiseSize, kNoiseSize, eTextureFormat::RGBA32_Float, false, 1, eTextureFlags::None, initData);
}

void SSAOScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_GBuffer", SCENE_PATH("GBufferShader.hlsl"));
    LoadProgram(m_graph, "P_Ssao", SCENE_PATH("SSAOShader.hlsl"));
    LoadProgram(m_graph, "P_Blur", SCENE_PATH("BlurShader.hlsl"));
    LoadProgram(m_graph, "P_Composite", SCENE_PATH("CompositeShader.hlsl"));

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Ssao", sizeof(CBSsao));
    m_graph.EmplaceConstantBuffer("CB_Composite", sizeof(CBComposite));

    m_entityCbh    = m_graph.GetConstantBuffer("CB_Entity");
    m_cameraCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_ssaoCbh      = m_graph.GetConstantBuffer("CB_Ssao");
    m_compositeCbh = m_graph.GetConstantBuffer("CB_Composite");

    m_graph.EmplaceTexture2D("T_GColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GNormal", 1.f, eTextureFormat::RGBA16_Float, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceTexture2D("T_Ssao", 1.f, eTextureFormat::R8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SsaoBlur", 1.f, eTextureFormat::R8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);

    {
        AttachmentRG attachments[3] = {};
        attachments[0].name         = "T_GColor";
        attachments[1].name         = "T_GNormal";
        attachments[2].name         = "T_GDepth";
        m_graph.EmplaceFrameBuffer("FB_GBuffer", attachments);
    }
    m_graph.EmplaceFrameBuffer("FB_Ssao", "T_Ssao");
    m_graph.EmplaceFrameBuffer("FB_SsaoBlur", "T_SsaoBlur");
    m_graph.EmplaceFrameBuffer("FB_Scene", "T_SceneColor");

    // g-buffer 패스
    m_graph.AddRenderPass("Pass_GBuffer")
        .SetFrameBuffer("FB_GBuffer")
        .SetProgram("P_GBuffer")
        .SetRenderTargetClearCommand(RGBA::kBlack, 0)
        .SetRenderTargetClearCommand(RGBA::kBlack, 1)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetBlendFlags(kOpaqueBlend, 1)
        .SetSampler(kLinearWrap, eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetExecutor(Bind<&SSAOScene::SubmitGBuffer_>(this));

    // ssao 패스
    m_graph.AddRenderPass("Pass_Ssao")
        .SetFrameBuffer("FB_Ssao")
        .SetProgram("P_Ssao")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_GNormal", eShader::PixelShader, 0)
        .SetTexture("T_GDepth", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Ssao", eShader::PixelShader, CBSsao::kSlot)
        .SetExecutor(Bind<&SSAOScene::SubmitSsao_>(this));

    // 블러 패스
    m_graph.AddRenderPass("Pass_Blur")
        .SetFrameBuffer("FB_SsaoBlur")
        .SetProgram("P_Blur")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_Ssao", eShader::PixelShader, 0)
        .SetExecutor(Bind<&SSAOScene::SubmitScreenTriangle_>(this));

    // 합성 패스
    m_graph.AddRenderPass("Pass_Composite")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Composite")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_GColor", eShader::PixelShader, 0)
        .SetTexture("T_GNormal", eShader::PixelShader, 1)
        .SetTexture("T_GDepth", eShader::PixelShader, 2)
        .SetTexture("T_SsaoBlur", eShader::PixelShader, 3)
        .SetConstantBuffer("CB_Composite", eShader::PixelShader, CBComposite::kSlot)
        .SetExecutor(Bind<&SSAOScene::SubmitScreenTriangle_>(this));

    // blit
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void SSAOScene::DrawNode_(
    const ModelNode& _node,
    const Mtx4&      _parentWorld) const
{
    Graphics& gfx = GetGraphics();

    const Mtx4 world = _node.localMtx * _parentWorld;

    const Int32 numMeshes = static_cast<Int32>(_node.meshIdxes.size());
    for (Int32 i = 0; i < numMeshes; ++i)
    {
        const Ref<Mesh>&     pMesh     = m_pModel->GetMeshes()[_node.meshIdxes[i]];
        const Ref<Material>& pMaterial = m_pModel->GetMaterials()[_node.matIdxes[i]];

        CBEntity entity;
        entity.world         = world.Transpose();
        entity.invTransWorld = world.Inverse();
        entity.baseColor     = pMaterial->GetParams().albedo;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

        pMaterial->GetTexture(eMaterialTexture::Albedo)->Bind(eShader::PixelShader, 0);
        gfx.SetRenderFlags(pMaterial->GetParams().bDoubleSided ? kNoCullDepthTest : kBackCullDepthTest);
        pMesh->Submit();
    }

    for (const ModelNode& child: _node.children)
    {
        DrawNode_(child, world);
    }
}

void SSAOScene::SubmitGBuffer_() const
{
    DrawNode_(m_pModel->GetRootNode(), Mtx4::kIdentity);
}

void SSAOScene::SubmitSsao_() const
{
    GetGraphics().SetTexture(m_noiseTexh, eShader::PixelShader, 2);
    GetImageFilterManager().SubmitScreenTriangle();
}

void SSAOScene::SubmitScreenTriangle_() const
{
    GetImageFilterManager().SubmitScreenTriangle();
}
