#include "pch.h"
#include "TextureMappingScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include "../JamEngine/ModelImporter.h"

#define SCENE_PATH(_filepath) "Resources/TextureMappingScene/" _filepath

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

}   // namespace

TextureMappingScene::TextureMappingScene()
{
    InitMeshes_();
    InitRenderGraph_();

    m_camera.transform.SetTranslationZ(-10.f);
}

TextureMappingScene::~TextureMappingScene()
{
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

    CBCamera camera;
    camera.eyePos   = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBLight light;
    light.lightDir     = (m_lightLookAt - m_lightPos).Normalized();
    light.lightColor   = m_lightColor;
    light.ambientColor = m_ambientColor;
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void TextureMappingScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("TextureMappingScene"))
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
    m_camera.SetAspectRatioFromBackBuffer();
}

void TextureMappingScene::InitMeshes_()
{
    // load mesh
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::Tangent, eAttributeFormat::Float3);
    m_pMesh = LoadMesh(SCENE_PATH("Model.obj"), attribLayout);

    // load textures
    m_pAlbedo = LoadTexture(SCENE_PATH("Albedo.png"));
    m_pAo     = LoadTexture(SCENE_PATH("Ao.png"));
    m_pNormal = LoadTexture(SCENE_PATH("Normal.png"));
}

void TextureMappingScene::InitRenderGraph_()
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

    LoadProgram(m_graph, "P_Main", SCENE_PATH("TextureMappingShader.hlsl"));

    m_graph.AddRenderPass("Pass_Main")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Main")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.2f, 0.2f, 0.2f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kLinearWrap, eShader::PixelShader, kLinearSamplerSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
        .SetExecutor(Bind<&TextureMappingScene::Submit_>(this));

    ImageFilterManager& ifm      = GetImageFilterManager();
    ImageFilterInfo     blitInfo = {};
    blitInfo.filter              = eImageFilter::Blit;
    blitInfo.output              = eImageFilterOutput::Final;
    blitInfo.input0Texture       = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void TextureMappingScene::Submit_() const
{
    Graphics& gfx = GetGraphics();

    m_pAlbedo->Bind(eShader::PixelShader, kAlbedoTextureSlot);
    m_pAo->Bind(eShader::PixelShader, kAoTextureSlot);
    m_pNormal->Bind(eShader::PixelShader, kNormalTextureSlot);

    CBEntity entity;
    entity.world = Mtx4::MakeRotationY(m_rot).Transpose();
    entity.flags = m_textureMappingFlags.GetValue();
    gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

    m_pMesh->Submit();
}
