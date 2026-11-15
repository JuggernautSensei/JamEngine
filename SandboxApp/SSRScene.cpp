#include "pch.h"
#include "SSRScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "Utilities.h"
#include "RenderStates.h"

#define SCENE_PATH(_filepath) "Resources/SSRScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec3  baseColor;
    float reflectivity;
    float bUseTexture;
    float padding[3];
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Mtx4 view;
    Mtx4 viewProj;
};

struct CBLighting
{
    static constexpr Int32 kSlot = 0;

    Vec3  lightDirV;
    float ambient;
    Vec3  skyColor;
    float padding;
};

struct CBSsr
{
    static constexpr Int32 kSlot = 0;

    Mtx4  proj;
    Mtx4  invProj;
    Vec2  screenSize;
    float maxDistance;
    float thickness;
    float steps;
    float edgeFade;
    float bSkyFallback;
    float padding1;
    Vec3  skyColor;
    float padding2;
};

struct CBComposite
{
    static constexpr Int32 kSlot = 0;

    float bUseSsr;
    float bReflectionOnly;
    float strength;
    float padding;
};

constexpr Vec3 kLightDirW = { 0.4f, 0.9f, -0.3f };
constexpr Vec3 kSkyColor  = { 0.25f, 0.35f, 0.5f };

}   // namespace

SSRScene::SSRScene()
{
    InitObjects_();
    InitRenderGraph_();

    m_camera.camera.SetFarZ(100.f);
    m_camera.dMove = 5.f;
    m_camera.transform.SetTranslation({ 0.f, 2.2f, -7.f });
    m_camera.euler.pitch = ToRad(12.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

SSRScene::~SSRScene()
{
}

void SSRScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_counter += _deltaTime;

        // sphere. 원을 그리며 움직임
        m_objects[m_sphereIndex].transform.SetTranslation({ Cos(m_counter) * 3.f, 0.6f, 2.f + Sin(m_counter) * 2.f });

        // cube. 회전만
        m_objects[m_cubeIndex].transform.SetRotation({ 0.f, m_counter * 0.8f, 0.f });
    }

    m_objects[0].reflectivity = m_floorReflectivity;
}

void SSRScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.view     = m_camera.GetView().Transpose();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cameraCbh, AsMemoryView(camera));

    CBLighting lighting = {};
    lighting.lightDirV  = m_camera.GetView().MultVector(kLightDirW.Normalized()).Normalized();
    lighting.ambient    = m_ambient;
    lighting.skyColor   = kSkyColor;
    gfx.UpdateBuffer(m_lightingCbh, AsMemoryView(lighting));

    CBSsr ssr        = {};
    ssr.proj         = m_camera.GetProj().Transpose();
    ssr.invProj      = m_camera.GetInvProj().Transpose();
    ssr.screenSize   = { static_cast<float>(_view.viewport.width), static_cast<float>(_view.viewport.height) };
    ssr.maxDistance  = m_maxDistance;
    ssr.thickness    = m_thickness;
    ssr.steps        = static_cast<float>(m_steps);
    ssr.edgeFade     = m_edgeFade;
    ssr.bSkyFallback = m_bSkyFallback ? 1.f : 0.f;
    ssr.skyColor     = kSkyColor;
    gfx.UpdateBuffer(m_ssrCbh, AsMemoryView(ssr));

    CBComposite composite     = {};
    composite.bUseSsr         = m_bUseSsr ? 1.f : 0.f;
    composite.bReflectionOnly = m_bReflectionOnly ? 1.f : 0.f;
    composite.strength        = m_strength;
    gfx.UpdateBuffer(m_compositeCbh, AsMemoryView(composite));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void SSRScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("SSRScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Animate", &m_bAnimate);

    // ssr
    ImGui::SeparatorText("SSR");
    ImGui::Checkbox("Enable SSR", &m_bUseSsr);
    ImGui::Checkbox("Show Reflection Only", &m_bReflectionOnly);
    ImGui::Checkbox("Sky Fallback", &m_bSkyFallback);
    ImGui::SliderFloat("Max Distance", &m_maxDistance, 1.f, 60.f);
    ImGui::SliderInt("Max Steps", &m_steps, 32, 2048);
    ImGui::SliderFloat("Thickness", &m_thickness, 0.05f, 1.5f);
    ImGui::SliderFloat("Edge Fade", &m_edgeFade, 0.01f, 0.5f);
    ImGui::SliderFloat("Strength", &m_strength, 0.f, 1.5f);

    // material
    ImGui::SeparatorText("Material");
    ImGui::SliderFloat("Floor Reflectivity", &m_floorReflectivity, 0.f, 1.f);

    // light
    ImGui::SeparatorText("Light");
    ImGui::SliderFloat("Ambient", &m_ambient, 0.05f, 0.8f);

    ImGui::End();
}

void SSRScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void SSRScene::InitObjects_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    // floor
    {
        Object& floor      = m_objects.emplace_back();
        floor.pMesh        = CreatePlaneMesh({ 40.f, 40.f }, { 0.f, 0.f }, { 20.f, 20.f }, Vec3::kUp, layout);
        floor.pTexture     = LoadTexture(TEXTURE_PATH("TileAlbedo.png"));
        floor.color        = Vec3 { 0.55f };
        floor.reflectivity = m_floorReflectivity;
    }

    // dragon
    {
        Object& dragon  = m_objects.emplace_back();
        dragon.pMesh    = LoadMesh(MODEL_PATH("StanfordDragonPBR/StanfordDragonPBR.gltf"), layout);
        dragon.pTexture = LoadTexture(MODEL_PATH("StanfordDragonPBR/Albedo.jpg"));
        dragon.transform.SetScale(0.01f);
        dragon.transform.SetTranslation({ -2.5f, 0.f, 2.f });
        dragon.transform.SetRotation({ 0.f, ToRad(160.f), 0.f });
    }

    // bunny
    {
        Object& bunny = m_objects.emplace_back();
        bunny.pMesh   = LoadMesh(MODEL_PATH("StanfordBunny.obj"), layout, true);
        bunny.transform.SetScale(10.f);
        bunny.transform.SetTranslation({ 2.5f, -0.333f, 2.f });
        bunny.color = Vec3 { 0.9f, 0.75f, 0.6f };
    }

    // sphere
    {
        m_sphereIndex = static_cast<Int32>(m_objects.size());

        Object& sphere = m_objects.emplace_back();
        sphere.pMesh   = CreateSphereMesh(0.6f, 5, layout);
        sphere.color   = Vec3 { 0.9f, 0.2f, 0.2f };
    }

    // cube
    {
        m_cubeIndex = static_cast<Int32>(m_objects.size());

        Object& cube = m_objects.emplace_back();
        cube.pMesh   = CreateCubeMesh(Vec3 { 0.6f }, layout);
        cube.color   = Vec3 { 0.2f, 0.4f, 0.9f };
        cube.transform.SetTranslation({ -0.5f, 0.6f, -0.5f });
    }
}

void SSRScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_GBuffer", SCENE_PATH("GBufferShader.hlsl"));
    LoadProgram(m_graph, "P_Lighting", SCENE_PATH("LightingShader.hlsl"));
    LoadProgram(m_graph, "P_Ssr", SCENE_PATH("SSRShader.hlsl"));
    LoadProgram(m_graph, "P_Composite", SCENE_PATH("CompositeShader.hlsl"));

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Lighting", sizeof(CBLighting));
    m_graph.EmplaceConstantBuffer("CB_Ssr", sizeof(CBSsr));
    m_graph.EmplaceConstantBuffer("CB_Composite", sizeof(CBComposite));

    m_entityCbh    = m_graph.GetConstantBuffer("CB_Entity");
    m_cameraCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_lightingCbh  = m_graph.GetConstantBuffer("CB_Lighting");
    m_ssrCbh       = m_graph.GetConstantBuffer("CB_Ssr");
    m_compositeCbh = m_graph.GetConstantBuffer("CB_Composite");

    m_graph.EmplaceTexture2D("T_GColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GNormal", 1.f, eTextureFormat::RGBA16_Float, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_GDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);
    m_graph.EmplaceTexture2D("T_Lit", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_Reflection", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_Final", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);

    {
        AttachmentRG attachments[3] = {};
        attachments[0].name         = "T_GColor";
        attachments[1].name         = "T_GNormal";
        attachments[2].name         = "T_GDepth";
        m_graph.EmplaceFrameBuffer("FB_GBuffer", attachments);
    }
    m_graph.EmplaceFrameBuffer("FB_Lit", "T_Lit");
    m_graph.EmplaceFrameBuffer("FB_Reflection", "T_Reflection");
    m_graph.EmplaceFrameBuffer("FB_Final", "T_Final");

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
        .SetExecutor(Bind<&SSRScene::SubmitGBuffer_>(this));

    // 조명 패스. 반사 없는 장면
    m_graph.AddRenderPass("Pass_Lighting")
        .SetFrameBuffer("FB_Lit")
        .SetProgram("P_Lighting")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_GColor", eShader::PixelShader, 0)
        .SetTexture("T_GNormal", eShader::PixelShader, 1)
        .SetTexture("T_GDepth", eShader::PixelShader, 2)
        .SetConstantBuffer("CB_Lighting", eShader::PixelShader, CBLighting::kSlot)
        .SetExecutor(Bind<&SSRScene::SubmitScreenTriangle_>(this));

    // ssr 패스
    m_graph.AddRenderPass("Pass_Ssr")
        .SetFrameBuffer("FB_Reflection")
        .SetProgram("P_Ssr")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_GNormal", eShader::PixelShader, 0)
        .SetTexture("T_GDepth", eShader::PixelShader, 1)
        .SetTexture("T_Lit", eShader::PixelShader, 2)
        .SetConstantBuffer("CB_Ssr", eShader::PixelShader, CBSsr::kSlot)
        .SetExecutor(Bind<&SSRScene::SubmitScreenTriangle_>(this));

    // 합성 패스
    m_graph.AddRenderPass("Pass_Composite")
        .SetFrameBuffer("FB_Final")
        .SetProgram("P_Composite")
        .SetRenderFlags(eRenderFlags::None)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetTexture("T_Lit", eShader::PixelShader, 0)
        .SetTexture("T_Reflection", eShader::PixelShader, 1)
        .SetTexture("T_GNormal", eShader::PixelShader, 2)
        .SetConstantBuffer("CB_Composite", eShader::PixelShader, CBComposite::kSlot)
        .SetExecutor(Bind<&SSRScene::SubmitScreenTriangle_>(this));

    // blit
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_Final";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void SSRScene::SubmitGBuffer_() const
{
    Graphics& gfx = GetGraphics();

    for (const Object& object: m_objects)
    {
        const Mtx4& world = object.transform.GetSRT();

        CBEntity entity;
        entity.world         = world.Transpose();
        entity.invTransWorld = world.Inverse();
        entity.baseColor     = object.color;
        entity.reflectivity  = object.reflectivity;
        entity.bUseTexture   = object.pTexture ? 1.f : 0.f;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

        if (object.pTexture)
        {
            object.pTexture->Bind(eShader::PixelShader, 0);
        }
        object.pMesh->Submit();
    }
}

void SSRScene::SubmitScreenTriangle_() const
{
    GetImageFilterManager().SubmitScreenTriangle();
}
