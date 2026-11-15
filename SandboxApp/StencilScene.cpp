#include "pch.h"
#include "StencilScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "Utilities.h"
#include "RenderStates.h"

#define SCENE_PATH(_filepath) "Resources/StencilScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Mtx4  invTransWorld;
    Vec3  baseColor;
    float alpha;
    float clipEnabled;
    float clipHeight;
    float outlineWidth;
    float padding1;
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eye;
    float padding1;
    Mtx4  viewProj;
};

struct CBLight
{
    static constexpr Int32 kSlot = 2;

    Vec3  lightDir;
    float padding2;
    Vec3  lightColor;
    float padding3;
    Vec3  ambientColor;
    float padding4;
};

constexpr float kMirrorHeight = 0.f;

constexpr UInt8 kStencilSelected = 1;
constexpr UInt8 kStencilMirror   = 2;

constexpr Flags<eStencilFlags> kStencilWriteRef =
    eStencilFlags::CompareFunc_Always
    | eStencilFlags::PassOp_Replace;

constexpr Flags<eStencilFlags> kStencilTestEqual =
    eStencilFlags::CompareFunc_Equal;

constexpr Flags<eStencilFlags> kStencilTestEqualAndIncr =
    eStencilFlags::CompareFunc_Equal
    | eStencilFlags::PassOp_Incr;

constexpr Flags<eStencilFlags> kStencilTestNotEqual =
    eStencilFlags::CompareFunc_NotEqual;

constexpr Flags<eRenderFlags> kBackCullNoDepthWrite =
    eRenderFlags::CullMode_Back
    | eRenderFlags::DepthFunc_Less;

constexpr Flags<eRenderFlags> kFrontCullNoDepthWrite =
    eRenderFlags::CullMode_Front
    | eRenderFlags::DepthFunc_Less;

}   // namespace

StencilScene::StencilScene()
{
    InitMeshes_();
    InitRenderGraph_();

    m_camera.transform.SetTranslation({ 0.f, 2.5f, -7.f });
    m_camera.euler.pitch = ToRad(15.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

StencilScene::~StencilScene()
{
}

void StencilScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimate)
    {
        m_counter += _deltaTime;

        // cube. 회전만
        Vec3 cubeR  = m_transforms[eObject::Cube].ToEuler();
        cubeR.pitch = ToRad(45.f) * m_counter;
        cubeR.yaw   = ToRad(45.f) * m_counter;
        m_transforms[eObject::Cube].SetRotation(cubeR);

        // sphere. 위 아래로 왔다갔다.
        Vec3 sphereT = m_transforms[eObject::Sphere].GetTranslation();
        sphereT.y    = 1.2f + Sin(2.f * m_counter) * 0.5f;
        m_transforms[eObject::Sphere].SetTranslation(sphereT);

        // bunny. 회전만
        Vec3 bunnyR = m_transforms[eObject::Bunny].ToEuler();
        bunnyR.yaw  = ToRad(30.f) * m_counter;
        m_transforms[eObject::Bunny].SetRotation(bunnyR);
    }
}

void StencilScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eye      = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBLight light;
    light.lightDir     = (m_lightLookAt - m_lightPos).Normalized();
    light.lightColor   = m_lightCol;
    light.ambientColor = m_ambientCol;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void StencilScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("StencilScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Animate", &m_bAnimate);

    // outline
    ImGui::SeparatorText("Outline");
    ImGui::Checkbox("Bunny", &m_bOutline[eObject::Bunny]);
    ImGui::SameLine();
    ImGui::Checkbox("Sphere", &m_bOutline[eObject::Sphere]);
    ImGui::SliderFloat("Width", &m_outlineWidth, 0.f, 0.15f);
    ImGui::ColorEdit3("Color", &m_outlineColor.x);

    // mirror
    ImGui::SeparatorText("Mirror");
    ImGui::Checkbox("Enable Mirror", &m_bMirror);
    ImGui::SliderFloat("Reflectivity", &m_mirrorReflectivity, 0.f, 1.f);
    ImGui::ColorEdit3("Floor Color", &m_floorColor.x);

    ImGui::End();
}

void StencilScene::InitMeshes_()
{
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    // bunny
    m_pObjects[eObject::Bunny] = LoadMesh("Resources/Models/StanfordBunny.obj", attribLayout, true);
    m_transforms[eObject::Bunny].SetScale(10.f);
    m_transforms[eObject::Bunny].SetTranslation({ 0.f, -0.333f, 0.f });
    m_colors[eObject::Bunny] = { 0.9f, 0.8f, 0.7f };

    // sphere
    m_pObjects[eObject::Sphere] = CreateSphereMesh(0.5f, 5, attribLayout);
    m_transforms[eObject::Sphere].SetTranslation({ 2.5f, 1.2f, 0.f });
    m_colors[eObject::Sphere] = { 0.3f, 0.6f, 0.9f };

    // cube
    m_pObjects[eObject::Cube] = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);
    m_transforms[eObject::Cube].SetTranslation({ -2.5f, 1.f, 0.f });
    m_colors[eObject::Cube] = { 0.8f, 0.3f, 0.3f };

    // floor
    m_pFloor = CreatePlaneMesh({ 20.f, 20.f }, Vec3::kUp, attribLayout);
}

void StencilScene::InitRenderGraph_()
{
    LoadProgram(m_graph, "P_Lit", SCENE_PATH("StencilShader.hlsl"));
    {
        ShaderMacro macro[1] = {};
        macro[0].name        = "OUTLINE";
        LoadProgram(m_graph, "P_Outline", SCENE_PATH("StencilShader.hlsl"), eShaderFlags::Vertex | eShaderFlags::Pixel, macro);
    }

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLight));
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D24_UNorm_S8_UInt, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attchments[2] = {};
    attchments[0].name         = "T_SceneColor";
    attchments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attchments);

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh  = m_graph.GetConstantBuffer("CB_Light");

    // 일반 오브젝트
    m_graph.AddRenderPass("Pass_Opaque")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetStencilClearCommand(0)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitOpaque_>(this));

    // 외곽선 대상. 스텐실에 1을 기록
    m_graph.AddRenderPass("Pass_Selected")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullDepthTest)
        .SetStencilFlags(kStencilWriteRef, kStencilWriteRef, kStencilSelected)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitSelected_>(this));

    // 거울 마스크 패스. 2 기록
    m_graph.AddRenderPass("Pass_MirrorMask")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullNoDepthWrite)
        .SetStencilFlags(kStencilWriteRef, kStencilWriteRef, kStencilMirror)
        .SetBlendFlags(eBlendFlags::None, 0)
        .SetExecutor(Bind<&StencilScene::SubmitMirrorMask_>(this));

    // 반사된 오브젝트
    m_graph.AddRenderPass("Pass_ReflectedOpaque")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kFrontCullDepthTest)
        .SetStencilFlags(kStencilTestEqual, kStencilTestEqual, kStencilMirror)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitReflectedOpaque_>(this));

    // 반사된 선택 대상
    m_graph.AddRenderPass("Pass_ReflectedSelected")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kFrontCullDepthTest)
        .SetStencilFlags(kStencilTestEqualAndIncr, kStencilTestEqualAndIncr, kStencilMirror)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitReflectedSelected_>(this));

    // 반사된 외곽선
    m_graph.AddRenderPass("Pass_ReflectedOutline")
        .SetProgram("P_Outline")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullNoDepthWrite)
        .SetStencilFlags(kStencilTestEqual, kStencilTestEqual, kStencilMirror)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitReflectedOutline_>(this));

    // 바닥
    m_graph.AddRenderPass("Pass_Floor")
        .SetProgram("P_Lit")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kAlphaBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitFloor_>(this));

    // 외곽선
    m_graph.AddRenderPass("Pass_Outline")
        .SetProgram("P_Outline")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, 1)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, 2)
        .SetRenderFlags(kFrontCullNoDepthWrite)
        .SetStencilFlags(kStencilTestNotEqual, kStencilTestNotEqual, kStencilSelected)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&StencilScene::SubmitOutline_>(this));

    // blit
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

bool StencilScene::IsSelected_(
    const eObject _object) const
{
    return _object != eObject::Cube && m_bOutline[_object];   // cube has hard edges
}

void StencilScene::DrawObject_(
    const eObject     _object,
    const DrawParams& _params) const
{
    Graphics& gfx = GetGraphics();

    Mtx4 world = m_transforms[_object].GetSRT();
    if (_params.bReflect)
    {
        world = world * Mtx4::MakeScale({ 1.f, -1.f, 1.f });
    }

    CBEntity entity;
    entity.world         = world.Transpose();
    entity.invTransWorld = world.Inverse();
    entity.baseColor     = _params.color;
    entity.alpha         = _params.alpha;
    entity.clipEnabled   = _params.bReflect ? 1.f : 0.f;
    entity.clipHeight    = kMirrorHeight;
    entity.outlineWidth  = _params.outlineWidth;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    m_pObjects[_object]->Submit();
}

void StencilScene::DrawFloor_(
    const DrawParams& _params) const
{
    Graphics& gfx = GetGraphics();

    const Mtx4 world = Mtx4::MakeTranslation({ 0.f, kMirrorHeight, 0.f });

    CBEntity entity;
    entity.world         = world.Transpose();
    entity.invTransWorld = world.Inverse();
    entity.baseColor     = _params.color;
    entity.alpha         = _params.alpha;
    entity.clipEnabled   = 0.f;
    entity.clipHeight    = 0.f;
    entity.outlineWidth  = 0.f;
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
    m_pFloor->Submit();
}

void StencilScene::DrawGroup_(
    const bool _bSelected,
    const bool _bReflect) const
{
    for (Int32 i = 0; i < eObject::Count; ++i)
    {
        const eObject object = static_cast<eObject>(i);
        if (IsSelected_(object) != _bSelected)
        {
            continue;
        }

        DrawParams params = {};
        params.color      = m_colors[i];
        params.bReflect   = _bReflect;
        DrawObject_(object, params);
    }
}

void StencilScene::DrawOutline_(
    const bool _bReflect) const
{
    for (Int32 i = 0; i < eObject::Count; ++i)
    {
        const eObject object = static_cast<eObject>(i);
        if (!IsSelected_(object))
        {
            continue;
        }

        DrawParams params   = {};
        params.color        = m_outlineColor;
        params.bReflect     = _bReflect;
        params.outlineWidth = m_outlineWidth;
        DrawObject_(object, params);
    }
}

void StencilScene::SubmitOpaque_() const
{
    DrawGroup_(false, false);
}

void StencilScene::SubmitSelected_() const
{
    DrawGroup_(true, false);
}

void StencilScene::SubmitMirrorMask_() const
{
    if (!m_bMirror)
    {
        return;
    }

    DrawParams params = {};
    params.color      = m_floorColor;
    DrawFloor_(params);
}

void StencilScene::SubmitReflectedOpaque_() const
{
    if (m_bMirror)
    {
        DrawGroup_(false, true);
    }
}

void StencilScene::SubmitReflectedSelected_() const
{
    if (m_bMirror)
    {
        DrawGroup_(true, true);
    }
}

void StencilScene::SubmitReflectedOutline_() const
{
    if (m_bMirror)
    {
        DrawOutline_(true);
    }
}

void StencilScene::SubmitFloor_() const
{
    DrawParams params = {};
    params.color      = m_floorColor;
    params.alpha      = m_bMirror ? (1.f - m_mirrorReflectivity) : 1.f;
    DrawFloor_(params);
}

void StencilScene::SubmitOutline_() const
{
    DrawOutline_(false);
}

void StencilScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}
