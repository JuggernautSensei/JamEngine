#include "pch.h"
#include "DeferredScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/EditorWidgets.h"

#define SCENE_PATH(_filepath) "Resources/DeferredScene/" _filepath

namespace
{

constexpr Int32 kAlbedoSlot            = 0;
constexpr Int32 kNormalSlot            = 1;
constexpr Int32 kDepthSlot             = 2;
constexpr Int32 kColorSlot             = 3;
constexpr Int32 kNormalSceneSlot       = 4;
constexpr Int32 kLightSlot             = 5;
constexpr Int32 kLinearWrapSamplerSlot = 1;

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    Mtx4   invTransWorld;
    UInt32 bHasTexture;
    float  padding[3];
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4  viewProj;
    Mtx4  invViewProj;
    Vec3  eyePos;
    float padding;
};

struct CBLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightPos;
    float lightRadius;
    Vec3  lightColor;
    float padding;
};

}   // namespace

void DeferredScene::InitMeshes_()
{
    Graphics& gfx = GetGraphics();

    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    // dragon
    m_pDragon       = LoadMesh(MODEL_PATH("StanfordDragonPBR/StanfordDragonPBR.gltf"), attribLayout);
    m_pDragonAlbedo = LoadTexture(MODEL_PATH("StanfordDragonPBR/Albedo.jpg"));
    m_pDragonNormal = LoadTexture(MODEL_PATH("StanfordDragonPBR/NormalGL.png"));

    // bunny
    m_pBunny       = LoadMesh(MODEL_PATH("StanfordBunnyPBR/StanfordBunnyPBR.gltf"), attribLayout);
    m_pBunnyAlbedo = LoadTexture(MODEL_PATH("StanfordBunnyPBR/Albedo.png"));
    m_pBunnyNormal = LoadTexture(MODEL_PATH("StanfordBunnyPBR/NormalGL.png"));

    m_pPlane = CreatePlaneMesh({ 10.f, 10.f }, -Vec3::kForward, attribLayout);
}

DeferredScene::DeferredScene()
{
    InitMeshes_();
    InitRenderGraph_();

    m_camera.transform.SetTranslationZ(-15.f);
}

DeferredScene::~DeferredScene() noexcept
{
}

void DeferredScene::OnUpdate(const float _deltaSec)
{
    m_counter += _deltaSec;
    m_camera.Update(_deltaSec);

    if (m_bAnimate)
    {
        m_rot += _deltaSec * m_dRot;
    }
}

void DeferredScene::OnRender(const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.viewProj    = m_camera.CalcViewProj().Transpose();
    camera.invViewProj = m_camera.CalcInvViewProj().Transpose();
    camera.eyePos      = m_camera.GetEyePos();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(camera));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void DeferredScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("DeferredScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::SliderInt("Light Count", &m_numLights, 1, kMaxLights);
    ImGui::Checkbox("Use Scissor", &m_bUseScissor);

    ImGui::End();
}

void DeferredScene::OnResize(const Int32 _width, const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void DeferredScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_GColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GNormal", 1.f, eTextureFormat::RG11B10_Float, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GDepth", 1.f, eTextureFormat::D24_UNorm_S8_UInt, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceTexture2D("T_Light", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);

    {
        AttachmentRG attachments[3] = {};
        attachments[0].name         = "T_GColor";
        attachments[1].name         = "T_GNormal";
        attachments[2].name         = "T_GDepth";
        m_graph.EmplaceFrameBuffer("FB_GBuffer", attachments);
    }

    m_graph.EmplaceFrameBuffer("FB_Light", "T_Light");

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLight));

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cameraCbh = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh  = m_graph.GetConstantBuffer("CB_Light");

    LoadProgram(m_graph, "P_GBuffer", SCENE_PATH("GBufferShader.hlsl"));
    LoadProgram(m_graph, "P_Light", SCENE_PATH("LightingShader.hlsl"));

    m_graph.AddRenderPass("Pass_GBuffer")
        .SetFrameBuffer("FB_GBuffer")
        .SetProgram("P_GBuffer")
        .SetRenderTargetClearCommand(RGBA::kBlack, 0)
        .SetRenderTargetClearCommand(RGBA::kBlack, 1)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kLinearWrap, eShader::PixelShader, kLinearWrapSamplerSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetExecutor(Bind<&DeferredScene::SubmitGBuffer_>(this));

    m_graph.AddRenderPass("Pass_Light")
        .SetFrameBuffer("FB_Light")
        .SetProgram("P_Light")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.1f, 0.1f, 0.1f), 0)   // ambient
        .SetRenderFlags(kBackCull | eRenderFlags::Scissor)
        .SetBlendFlags(kAdditiveBlend, 0)
        .SetTexture("T_GColor", eShader::PixelShader, kColorSlot)
        .SetTexture("T_GNormal", eShader::PixelShader, kNormalSceneSlot)
        .SetTexture("T_GDepth", eShader::PixelShader, kDepthSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
        .SetExecutor(Bind<&DeferredScene::SubmitLights_>(this));

    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterDesc multDesc = {};
    multDesc.filter          = eImageFilter::Mult;
    multDesc.output          = eImageFilterOutput::Final;
    multDesc.input0Texture   = "T_GColor";
    multDesc.input1Texture   = "T_Light";
    ifm.AddImageFilter(m_graph, "Pass_Mult", multDesc);
}

void DeferredScene::SubmitGBuffer_() const
{
    Graphics&  gfx   = GetGraphics();
    const Vec3 euler = { 0.f, m_rot, 0.f };

    // dragon
    {
        constexpr Vec3 kS = { 0.01f, 0.01f, 0.01f };
        constexpr Vec3 kT = { -3.f, 0.f, 0.f };

        CBEntity cb;
        cb.world         = Mtx4::MakeSRT(kS, euler, kT).Transpose();
        cb.invTransWorld = Mtx4::MakeInvSRT(kS, euler, kT);
        cb.bHasTexture   = 1;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));

        m_pDragonAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
        m_pDragonNormal->Bind(eShader::PixelShader, kNormalSlot);
        m_pDragon->Submit();
    }

    // bunny
    {
        constexpr Vec3 kS = { 0.01f, 0.01f, 0.01f };
        constexpr Vec3 kT = { 3.f, 0.f, 0.f };

        CBEntity cb;
        cb.world         = Mtx4::MakeSRT(kS, euler, kT).Transpose();
        cb.invTransWorld = Mtx4::MakeInvSRT(kS, euler, kT);
        cb.bHasTexture   = 1;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));

        m_pBunnyAlbedo->Bind(eShader::PixelShader, kAlbedoSlot);
        m_pBunnyNormal->Bind(eShader::PixelShader, kNormalSlot);
        m_pBunny->Submit();
    }

    // plane
    {
        constexpr Vec3 kT = { 0.f, 0.f, 0.3f };

        CBEntity cb;
        cb.world         = Mtx4::MakeSRT(Vec3::kOne, Vec3::kZero, kT).Transpose();
        cb.invTransWorld = Mtx4::MakeInvSRT(Vec3::kOne, Vec3::kZero, kT);
        cb.bHasTexture   = 0;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(cb));

        m_pPlane->Submit();
    }
}

void DeferredScene::SubmitLights_() const
{
    Graphics&           gfx = GetGraphics();
    WindowsPlatform&    win = GetWindowsPlatform();
    ImageFilterManager& ifm = GetImageFilterManager();

    const Vec2i resolution = win.GetClientResolution();
    const float width      = static_cast<float>(resolution.x);
    const float height     = static_cast<float>(resolution.y);

    const Mtx4 vp = m_camera.CalcViewProj();

    CBLight light;
    for (Int32 i = 0; i < m_numLights; ++i)
    {
        const float fi     = static_cast<float>(i);
        const float t      = m_counter * 0.5f * (Sin(fi / static_cast<float>(m_numLights) * kHalfPi) * 0.5f + 0.5f);
        const float offset = 5.f;

        light.lightPos.x  = Sin((t + fi * 0.47f) + kHalfPi * 1.37f) * offset;
        light.lightPos.y  = Cos((t + fi * 0.69f) + kHalfPi * 1.49f) * offset;
        light.lightPos.z  = Sin((t + fi * 0.37f) + kHalfPi * 1.57f) * 2.f;
        light.lightRadius = 2.f;
        light.lightColor  = {
            Sin(fi * 0.3f) * 0.5f + 0.5f,
            Sin(fi * 0.7f + k2Pi / 3.f) * 0.5f + 0.5f,
            Sin(fi * 0.5f + k2Pi * 2.f / 3.f) * 0.5f + 0.5f,
        };

        Rect scissor = { 0, 0, static_cast<UInt16>(width), static_cast<UInt16>(height) };
        if (m_bUseScissor)
        {
            Vec3 corners[8] = {
                { light.lightPos.x - light.lightRadius, light.lightPos.y - light.lightRadius, light.lightPos.z - light.lightRadius },
                { light.lightPos.x + light.lightRadius, light.lightPos.y - light.lightRadius, light.lightPos.z - light.lightRadius },
                { light.lightPos.x - light.lightRadius, light.lightPos.y + light.lightRadius, light.lightPos.z - light.lightRadius },
                { light.lightPos.x + light.lightRadius, light.lightPos.y + light.lightRadius, light.lightPos.z - light.lightRadius },
                { light.lightPos.x - light.lightRadius, light.lightPos.y - light.lightRadius, light.lightPos.z + light.lightRadius },
                { light.lightPos.x + light.lightRadius, light.lightPos.y - light.lightRadius, light.lightPos.z + light.lightRadius },
                { light.lightPos.x - light.lightRadius, light.lightPos.y + light.lightRadius, light.lightPos.z + light.lightRadius },
                { light.lightPos.x + light.lightRadius, light.lightPos.y + light.lightRadius, light.lightPos.z + light.lightRadius },
            };

            Vec3 ndcMin = Vec3::kMax;
            Vec3 ndcMax = Vec3::kMin;
            for (Int32 j = 0; j < 8; ++j)
            {
                corners[j] = vp.MultPoint(corners[j]);
                ndcMin     = ndcMin.Min(corners[j]);
                ndcMax     = ndcMax.Max(corners[j]);
            }

            if (ndcMin.z > 1.f || ndcMax.z < 0.f)
            {
                continue;   // light is out of view frustum
            }

            const float x0 = Clamp((ndcMin.x * 0.5f + 0.5f) * width, 0.f, width);
            const float y0 = Clamp((-ndcMax.y * 0.5f + 0.5f) * height, 0.f, height);
            const float x1 = Clamp((ndcMax.x * 0.5f + 0.5f) * width, 0.f, width);
            const float y1 = Clamp((-ndcMin.y * 0.5f + 0.5f) * height, 0.f, height);

            // cpu clip
            if (x1 <= x0 || y1 <= y0)
            {
                continue;
            }

            scissor.left   = static_cast<UInt16>(x0);
            scissor.top    = static_cast<UInt16>(y0);
            scissor.right  = static_cast<UInt16>(x1);
            scissor.bottom = static_cast<UInt16>(y1);
        }

        gfx.SetScissor(scissor);
        gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));
        ifm.SubmitScreenTriangle();
    }
}