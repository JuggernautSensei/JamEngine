#include "pch.h"
#include "TextureScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/MeshFactory.h"

#define SCENE_PATH(_filepath) "Resources/TextureScene/" _filepath

namespace
{

constexpr Int32 kLinearSamplerSlot = 0;
constexpr Int32 kAlbedoTextureSlot = 0;
constexpr Int32 kAoTextureSlot     = 1;
constexpr Int32 kNormalTextureSlot = 2;

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    UInt32 flags;
    float  padding[3];
    Vec2   uvScale;
    float  padding2[2];
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eyePos;
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

enum eMaterial : Int32
{
    MaterialCobblestone,
    MaterialBrick,
    MaterialPlanks,
    MaterialRock,
    MaterialMetal,
    MaterialLeather,
    MaterialDenim,
    MaterialCount
};

constexpr const char* kMaterialNames[MaterialCount] = {
    "cobblestone_floor_01",
    "castle_brick_07",
    "brown_planks_03",
    "aerial_rocks_02",
    "blue_metal_plate",
    "brown_leather",
    "denim_fabric",
};

}   // namespace

TextureScene::TextureScene()
{
    InitMaterials_();
    InitObjects_();
    InitRenderGraph_();

    m_camera.camera.SetFarZ(100.f);
    m_camera.transform.SetTranslation({ 0.f, 4.8f, -13.f });
    m_camera.euler.pitch = ToRad(15.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

TextureScene::~TextureScene()
{
}

void TextureScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bAnimateObjects)
    {
        m_time += _deltaTime;
    }
    if (m_bAnimateLight)
    {
        m_lightAngle += _deltaTime * 0.5f;
    }
}

void TextureScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eyePos   = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    const Vec3 lightPos = { Cos(m_lightAngle) * 6.f, 3.f + m_lightHeight * 4.f, -2.f + Sin(m_lightAngle) * 4.f };
    CBLight    light;
    light.lightDir     = (Vec3 { 0.f, 1.f, 2.f } - lightPos).Normalized();
    light.lightColor   = m_lightColor;
    light.ambientColor = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void TextureScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("TextureScene"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Animate Objects", &m_bAnimateObjects);
    ImGui::Checkbox("Animate Light", &m_bAnimateLight);
    ImGui::Text("Phong shading / 16x anisotropic filtering");

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

    if (ImGui::CollapsingHeader("Light"))
    {
        ImGui::SliderFloat("Height", &m_lightHeight, 0.f, 1.f);
        ImGui::ColorEdit3("Light Color", &m_lightColor.x);
        ImGui::ColorEdit3("Ambient Color", &m_ambientColor.x);
    }

    ImGui::End();
}

void TextureScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void TextureScene::InitMaterials_()
{
    m_materials.resize(MaterialCount);
    for (Int32 i = 0; i < MaterialCount; ++i)
    {
        const String prefix = String::Formatted("{}{}", SCENE_PATH("Materials/"), kMaterialNames[i]);

        m_materials[i].pAlbedo = LoadTexture(String::Formatted("{}_Albedo.jpg", prefix));
        m_materials[i].pAo     = LoadTexture(String::Formatted("{}_Ao.jpg", prefix));
        m_materials[i].pNormal = LoadTexture(String::Formatted("{}_Normal.jpg", prefix));
    }
    for (const char* name: { "MarbleBust", "GothicStatue", "BrassVase", "Column" })
    {
        const String     path     = String::Formatted("{}Gallery/{}/", MODEL_PATH(""), name);
        SurfaceMaterial& material = m_materials.emplace_back();
        material.pAlbedo          = LoadTexture(path + "Albedo.jpg");
        material.pAo              = LoadTexture(path + "Ao.jpg");
        material.pNormal          = LoadTexture(path + "NormalDX.jpg");
    }
}

void TextureScene::InitObjects_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    layout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);

    const Ref<Mesh> pFloor  = CreatePlaneMesh({ 22.f, 20.f }, Vec3::kUp, layout);
    const Ref<Mesh> pWall   = CreatePlaneMesh({ 22.f, 8.f }, -Vec3::kForward, layout);
    const Ref<Mesh> pSphere = CreateSphereMesh(0.75f, 4, layout);
    const Ref<Mesh> pCube   = CreateCubeMesh(Vec3 { 1.4f }, layout);
    const Ref<Mesh> pColumn = LoadMesh(MODEL_PATH("Gallery/Column/Mesh.gltf"), layout);
    const Ref<Mesh> pBust   = LoadMesh(MODEL_PATH("Gallery/MarbleBust/Mesh.gltf"), layout);
    const Ref<Mesh> pStatue = LoadMesh(MODEL_PATH("Gallery/GothicStatue/Mesh.gltf"), layout);
    const Ref<Mesh> pVase   = LoadMesh(MODEL_PATH("Gallery/BrassVase/Mesh.gltf"), layout);

    auto add = [this](const Ref<Mesh>& _pMesh, const Int32 _material, const Vec3 _pos, const float _scale = 1.f, const float _spin = 0.f, const Vec2 _uv = { 1.f, 1.f }) {
        SceneObject& obj = m_objects.emplace_back();
        obj.pMesh        = _pMesh;
        obj.material     = _material;
        obj.position     = _pos;
        obj.scale        = _scale;
        obj.spinSpeed    = _spin;
        obj.uvScale      = _uv;
    };

    add(pFloor, MaterialCobblestone, { 0.f, 0.f, 2.f }, 1.f, 0.f, { 7.f, 6.f });
    add(pWall, MaterialBrick, { 0.f, 4.f, 9.f }, 1.f, 0.f, { 7.f, 3.f });
    for (const float x: { -8.5f, -5.5f, 5.5f, 8.5f })
    {
        add(pColumn, MaterialCount + 3, { x, 0.f, 6.4f }, 5.6f, 0.f, { 2.f, 2.f });
    }
    add(pStatue, MaterialCount + 1, { -4.8f, 0.f, 2.4f }, 4.6f);
    add(pBust, MaterialCount, { 0.f, 0.f, 3.f }, 3.8f);
    add(pVase, MaterialCount + 2, { 4.6f, 0.f, 2.4f }, 3.2f);
    add(pSphere, MaterialRock, { -5.f, 0.75f, -1.3f }, 1.f, 0.4f);
    add(pCube, MaterialPlanks, { -2.5f, 0.7f, -1.6f }, 1.f, -0.4f);
    add(pSphere, MaterialLeather, { 0.f, 0.75f, -1.3f }, 1.f, 0.4f);
    add(pSphere, MaterialMetal, { 2.5f, 0.75f, -1.6f }, 1.f, -0.4f);
    add(pCube, MaterialDenim, { 5.f, 0.7f, -1.3f }, 1.f, 0.5f);
}

void TextureScene::InitRenderGraph_()
{
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attachments[2] = {};
    attachments[0].name         = "T_SceneColor";
    attachments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attachments);

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_Light", sizeof(CBLight));

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh  = m_graph.GetConstantBuffer("CB_Light");

    LoadProgram(m_graph, "P_Main", SCENE_PATH("TextureShader.hlsl"));

    m_graph.AddRenderPass("Pass_Main")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Main")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.05f, 0.06f, 0.08f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kAnisotropicWrap, eShader::PixelShader, kLinearSamplerSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
        .SetExecutor(Bind<&TextureScene::Submit_>(this));

    ImageFilterManager& ifm      = GetImageFilterManager();
    ImageFilterInfo     blitInfo = {};
    blitInfo.filter              = eImageFilter::Blit;
    blitInfo.output              = eImageFilterOutput::Final;
    blitInfo.input0Texture       = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void TextureScene::Submit_() const
{
    Graphics& gfx = GetGraphics();

    for (const SceneObject& obj: m_objects)
    {
        const SurfaceMaterial& material = m_materials[obj.material];
        material.pAlbedo->Bind(eShader::PixelShader, kAlbedoTextureSlot);
        material.pAo->Bind(eShader::PixelShader, kAoTextureSlot);
        material.pNormal->Bind(eShader::PixelShader, kNormalTextureSlot);

        const float yaw = obj.baseEuler.y + m_time * obj.spinSpeed;

        CBEntity entity;
        entity.world   = Mtx4::MakeSRT(Vec3 { obj.scale }, Vec3 { obj.baseEuler.x, yaw, obj.baseEuler.z }, obj.position).Transpose();
        entity.flags   = m_textureMappingFlags.GetValue();
        entity.uvScale = obj.uvScale;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

        obj.pMesh->Submit();
    }
}
