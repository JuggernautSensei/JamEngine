#include "pch.h"
#include "SkinningScene.h"

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RenderStates.h"
#include "Utilities.h"
#include <filesystem>
#include "../JamEngine/MeshFactory.h"
#include "../JamEngine/ModelImporter.h"

#define SCENE_PATH(_filepath) "Resources/SkinningScene/" _filepath

namespace
{

constexpr Int32 kLinearSamplerSlot = 0;
constexpr Int32 kAlbedoTextureSlot = 0;
constexpr float kCentimeterToMeter = 0.01f;
constexpr float kWalkSpeed         = 1.6f;
constexpr float kTurnSpeed         = ToRad(540.f);
constexpr float kBlendDuration     = 0.2f;
const Vec3      kCameraOffset      = { 0.f, 2.4f, -5.8f };

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4   world;
    UInt32 bHasAlbedo;
    UInt32 bSkinned;
    Vec2   uvScale;
    Vec3   tint;
    float  padding;
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

struct CBSkin
{
    static constexpr Int32 kSlot = 3;

    Mtx4 joints[kMaxSkinJoints];
};

void CollectMeshMaterials(
    const ModelNode& _node,
    Vector<Int32>&   _meshMaterials)
{
    for (SizeT i = 0; i < _node.meshIdxes.size(); ++i)
    {
        _meshMaterials[_node.meshIdxes[i]] = _node.matIdxes[i];
    }
    for (const ModelNode& child: _node.children)
    {
        CollectMeshMaterials(child, _meshMaterials);
    }
}

}   // namespace

SkinningScene::SkinningScene()
{
    InitModel_();
    LoadStateClips_();
    InitNpcs_();
    InitRenderGraph_();

    m_camera.transform.SetTranslation(kCameraOffset);
    m_camera.camera.SetFarZ(150.f);
    m_camera.euler.pitch = ToRad(11.f);
    m_camera.transform.SetRotation(m_camera.euler);
}

SkinningScene::~SkinningScene()
{
}

void SkinningScene::OnUpdate(
    const float _deltaTime)
{
    UpdateInput_(_deltaTime);
    UpdatePose_(_deltaTime);
    m_npcTime += static_cast<double>(_deltaTime);
    UpdateNpcs_();

    m_camera.transform.SetTranslation(m_position + kCameraOffset);
}

void SkinningScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eyePos   = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    CBLight light;
    light.lightDir     = Vec3 { -0.3f, -0.6f, 0.7f }.Normalized();
    light.lightColor   = Vec3 { 1.f, 1.f, 1.f };
    light.ambientColor = Vec3 { 0.3f, 0.3f, 0.3f };
    gfx.UpdateBuffer(m_lightCbh, AsMemoryView(light));

    m_graph.Execute(_view.fbh, _view.viewport);
}

void SkinningScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("SkinningScene"))
    {
        ImGui::End();
        return;
    }

    constexpr const char* kStateNames[] = { "Idle", "Walk", "Slash" };

    ImGui::Text("State: %s", kStateNames[static_cast<Int32>(m_state)]);
    ImGui::Text("Position: %.2f, %.2f", m_position.x, m_position.z);
    ImGui::Separator();
    ImGui::TextUnformatted("WASD: Move");
    ImGui::TextUnformatted("Space: Slash");
    for (Int32 i = 0; i < 3; ++i)
    {
        if (!m_bHasStateClip[i])
        {
            ImGui::TextColored({ 1.f, 0.6f, 0.2f, 1.f }, "Missing clip: %s", kStateNames[i]);
        }
    }

    ImGui::End();
}

void SkinningScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void SkinningScene::InitModel_()
{
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    attribLayout.Add(eVertexAttribute::BlendIndex, eAttributeFormat::UInt8x4);
    attribLayout.Add(eVertexAttribute::BlendWeight, eAttributeFormat::Float4);

    const Flags<eAssimpImporterFlags> flags = eAssimpImporterFlags::LoadMeshes
                                            | eAssimpImporterFlags::LoadMaterials
                                            | eAssimpImporterFlags::LoadTextures
                                            | eAssimpImporterFlags::LoadAnimations
                                            | eAssimpImporterFlags::GenerateSmoothNormals;

    Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile(SCENE_PATH("Paladin_Walk.fbx"), flags);
    JAM_ASSERT(importer, "SkinningScene - Failed to import model. {}", importer.GetError());

    m_animation = importer->GetAnimationData();

    Expect<Ref<Model>> model = importer->ConvertToModel(attribLayout);
    JAM_ASSERT(model, "SkinningScene - Failed to convert model. {}", model.GetError());
    m_pModel = model.GetValue();

    m_meshMaterials.assign(m_pModel->GetMeshes().size(), -1);
    CollectMeshMaterials(m_pModel->GetRootNode(), m_meshMaterials);
}

void SkinningScene::InitRenderGraph_()
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
    m_graph.EmplaceConstantBuffer("CB_Skin", sizeof(CBSkin));

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_lightCbh  = m_graph.GetConstantBuffer("CB_Light");
    m_skinCbh   = m_graph.GetConstantBuffer("CB_Skin");

    LoadProgram(m_graph, "P_Main", SCENE_PATH("SkinningShader.hlsl"));

    m_graph.AddRenderPass("Pass_Main")
        .SetFrameBuffer("FB_Scene")
        .SetProgram("P_Main")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.32f, 0.42f, 0.58f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetSampler(kLinearWrap, eShader::PixelShader, kLinearSamplerSlot)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Skin", eShader::VertexShader, CBSkin::kSlot)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, CBEntity::kSlot)
        .SetConstantBuffer("CB_Camera", eShader::PixelShader, CBCamera::kSlot)
        .SetConstantBuffer("CB_Light", eShader::PixelShader, CBLight::kSlot)
        .SetExecutor(Bind<&SkinningScene::Submit_>(this));

    ImageFilterManager& ifm      = GetImageFilterManager();
    ImageFilterInfo     blitInfo = {};
    blitInfo.filter              = eImageFilter::Blit;
    blitInfo.output              = eImageFilterOutput::Final;
    blitInfo.input0Texture       = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void SkinningScene::LoadStateClips_()
{
    struct StateFile
    {
        eCharacterState state;
        const char*     path;
    };

    const StateFile files[] = {
        {  eCharacterState::Idle,  SCENE_PATH("Idle.fbx") },
        {  eCharacterState::Walk,  SCENE_PATH("Walk.fbx") },
        { eCharacterState::Slash, SCENE_PATH("Slash.fbx") },
    };

    Int32 hipsIndex = -1;
    for (Int32 i = 0; i < static_cast<Int32>(m_animation.skeleton.nodes.size()); ++i)
    {
        if (m_animation.skeleton.nodes[i].name.EndsWith("Hips"))
        {
            hipsIndex = i;
            break;
        }
    }

    const StringView hipsName = hipsIndex >= 0 ? StringView { m_animation.skeleton.nodes[hipsIndex].name.GetPtr(), m_animation.skeleton.nodes[hipsIndex].name.GetLength() } : StringView {};

    for (const StateFile& file: files)
    {
        const Int32 stateIdx = static_cast<Int32>(file.state);
        if (!std::filesystem::exists(file.path))
        {
            JAM_LOG_WARN("SkinningScene - Animation file not found: {}", file.path);
            continue;
        }

        Expect<AssimpImporter> importer = AssimpImporter::ImportFromFile(file.path, eAssimpImporterFlags::LoadAnimations);
        if (!importer || importer->GetAnimationData().clips.empty())
        {
            JAM_LOG_WARN("SkinningScene - Failed to load animation: {}", file.path);
            continue;
        }

        const SkeletalAnimationData& data = importer->GetAnimationData();
        AnimationClip                clip = RetargetClipByName(data.skeleton, data.clips[0], m_animation.skeleton, hipsName);
        LockHorizontalTranslation(clip, hipsIndex);

        m_stateClips[stateIdx]    = std::move(clip);
        m_bHasStateClip[stateIdx] = true;
    }
}

const AnimationClip* SkinningScene::GetStateClip_(
    const eCharacterState _state) const
{
    const Int32 idx = static_cast<Int32>(_state);
    return m_bHasStateClip[idx] ? &m_stateClips[idx] : nullptr;
}

void SkinningScene::ChangeState_(
    const eCharacterState _state)
{
    if (_state == m_state)
    {
        return;
    }

    m_previousState = m_state;
    m_previousTime  = m_stateTime;
    m_blendElapsed  = 0.f;
    m_state         = _state;
    m_stateTime     = 0.0;
}

void SkinningScene::UpdateInput_(
    const float _deltaTime)
{
    Input& input = GetInput();

    if (m_state == eCharacterState::Slash)
    {
        const AnimationClip* pClip = GetStateClip_(eCharacterState::Slash);
        if (pClip && m_stateTime < pClip->duration)
        {
            return;
        }
        ChangeState_(eCharacterState::Idle);
    }

    if (input.IsKeyPressed(eKey::Space) && m_bHasStateClip[static_cast<Int32>(eCharacterState::Slash)])
    {
        ChangeState_(eCharacterState::Slash);
        return;
    }

    Vec3 dir = Vec3::kZero;
    if (input.IsKeyDown(eKey::W))
        dir.z += 1.f;
    if (input.IsKeyDown(eKey::S))
        dir.z -= 1.f;
    if (input.IsKeyDown(eKey::D))
        dir.x += 1.f;
    if (input.IsKeyDown(eKey::A))
        dir.x -= 1.f;

    if (dir.LengthSq() < kEpsilon)
    {
        ChangeState_(eCharacterState::Idle);
        return;
    }

    dir.Normalize();
    m_position += dir * kWalkSpeed * _deltaTime;
    ChangeState_(eCharacterState::Walk);

    const float targetYaw = std::atan2(-dir.x, -dir.z);
    const float delta     = std::remainder(targetYaw - m_yaw, k2Pi);
    const float maxStep   = kTurnSpeed * _deltaTime;
    m_yaw += std::clamp(delta, -maxStep, maxStep);
}

void SkinningScene::UpdatePose_(
    const float _deltaTime)
{
    m_stateTime += static_cast<double>(_deltaTime);

    const bool bLoop = m_state != eCharacterState::Slash;
    Error      err   = SampleLocalPose(m_animation.skeleton, GetStateClip_(m_state), m_stateTime, bLoop, m_currentPoses);
    JAM_ASSERT(!err, "SkinningScene::UpdatePose_() - Failed to sample pose. {}", err);

    Span<const JointPose> finalPoses = m_currentPoses;
    if (m_blendElapsed < kBlendDuration)
    {
        m_blendElapsed += _deltaTime;
        m_previousTime += static_cast<double>(_deltaTime);

        const bool bPrevLoop = m_previousState != eCharacterState::Slash;
        err                  = SampleLocalPose(m_animation.skeleton, GetStateClip_(m_previousState), m_previousTime, bPrevLoop, m_previousPoses);
        JAM_ASSERT(!err, "SkinningScene::UpdatePose_() - Failed to sample previous pose. {}", err);

        BlendLocalPoses(m_previousPoses, m_currentPoses, std::min(m_blendElapsed / kBlendDuration, 1.f), m_blendedPoses);
        finalPoses = m_blendedPoses;
    }

    err = LocalPosesToGlobals(m_animation.skeleton, finalPoses, m_globals);
    JAM_ASSERT(!err, "SkinningScene::UpdatePose_() - Failed to build globals. {}", err);
}

Mtx4 SkinningScene::CalcCharacterWorld_() const
{
    return MakeCharacterWorld_(m_position, m_yaw);
}

Mtx4 SkinningScene::MakeCharacterWorld_(
    const Vec3  _position,
    const float _yaw)
{
    const float s = std::sin(_yaw);
    const float c = std::cos(_yaw);

    const Vec4 right    = Vec4(c, 0.f, -s, 0.f);
    const Vec4 up       = Vec4(0.f, 1.f, 0.f, 0.f);
    const Vec4 forward  = Vec4(s, 0.f, c, 0.f);
    const Vec4 origin   = Vec4(0.f, 0.f, 0.f, 1.f);
    const Mtx4 rotation = Mtx4(right, up, forward, origin);
    return Mtx4::MakeScale(Vec3 { kCentimeterToMeter }) * rotation * Mtx4::MakeTranslation(_position);
}

void SkinningScene::InitNpcs_()
{
    VertexLayout layout = {};
    layout.Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    layout.Add(eVertexAttribute::BlendIndex, eAttributeFormat::UInt8x4);
    layout.Add(eVertexAttribute::BlendWeight, eAttributeFormat::Float4);
    m_pFloor        = CreatePlaneMesh({ 60.f, 60.f }, Vec3::kUp, layout);
    m_pFloorTexture = LoadTexture(SCENE_PATH("Floor_Albedo.jpg"));

    constexpr Int32 kColumns = 7;
    constexpr Int32 kRows    = 2;
    for (Int32 row = 0; row < kRows; ++row)
    {
        for (Int32 col = 0; col < kColumns; ++col)
        {
            const Int32 index = row * kColumns + col;

            Npc& npc   = m_npcs.emplace_back();
            npc.center = { (static_cast<float>(col) - 3.f) * 3.2f, 0.f, 5.5f + static_cast<float>(row) * 3.6f };
            npc.phase  = static_cast<float>(index) * 0.53f;
            npc.state  = index % 3 == 0 ? eCharacterState::Walk : index % 3 == 1 ? eCharacterState::Idle
                                                                                 : eCharacterState::Slash;
            npc.radius = npc.state == eCharacterState::Walk ? 1.1f : 0.f;
            npc.yaw    = (static_cast<float>(index % 5) - 2.f) * 0.12f;
        }
    }
}

void SkinningScene::UpdateNpcs_()
{
    for (Npc& npc: m_npcs)
    {
        const double time = m_npcTime + static_cast<double>(npc.phase);

        if (npc.state == eCharacterState::Walk)
        {
            const float angle = static_cast<float>(time) * (1.6f / npc.radius);
            npc.position      = npc.center + Vec3 { std::cos(angle) * npc.radius, 0.f, std::sin(angle) * npc.radius };
            npc.yaw           = std::atan2(std::sin(angle), -std::cos(angle));
        }
        else
        {
            npc.position = npc.center;
        }

        const Error err = SampleLocalPose(m_animation.skeleton, GetStateClip_(npc.state), time, true, m_npcPoses);
        JAM_ASSERT(!err, "SkinningScene::UpdateNpcs_() - Failed to sample pose. {}", err);
        const Error err2 = LocalPosesToGlobals(m_animation.skeleton, m_npcPoses, npc.globals);
        JAM_ASSERT(!err2, "SkinningScene::UpdateNpcs_() - Failed to build globals. {}", err2);
    }
}

void SkinningScene::DrawCharacter_(
    const Vector<Mtx4>& _globals,
    const Mtx4&         _world)
{
    Graphics&                    gfx       = GetGraphics();
    const Vector<Ref<Mesh>>&     meshes    = m_pModel->GetMeshes();
    const Vector<Ref<Material>>& materials = m_pModel->GetMaterials();

    for (SizeT i = 0; i < meshes.size(); ++i)
    {
        const MeshSkin& skin = m_animation.skeleton.meshSkins[i];
        if (skin.joints.empty())
        {
            continue;
        }

        const Error err = BuildSkinningPalette(skin, _globals, m_palette);
        JAM_ASSERT(!err, "SkinningScene::DrawCharacter_() - Failed to build skinning palette. {}", err);

        CBSkin cbSkin = {};
        for (SizeT j = 0; j < m_palette.size(); ++j)
        {
            cbSkin.joints[j] = m_palette[j].Transpose();
        }
        gfx.UpdateBuffer(m_skinCbh, AsMemoryView(cbSkin));

        CBEntity entity   = {};
        entity.world      = (_globals[skin.meshNodeIndex] * _world).Transpose();
        entity.bHasAlbedo = 0;
        entity.bSkinned   = 1;
        entity.uvScale    = { 1.f, 1.f };
        entity.tint       = { 0.8f, 0.8f, 0.8f };

        const Int32 materialIdx = m_meshMaterials[i];
        if (materialIdx >= 0 && materialIdx < static_cast<Int32>(materials.size()))
        {
            const Ref<Texture>& pAlbedo = materials[materialIdx]->GetTexture(eMaterialTexture::Albedo);
            if (pAlbedo)
            {
                pAlbedo->Bind(eShader::PixelShader, kAlbedoTextureSlot);
                entity.bHasAlbedo = 1;
                entity.tint       = Vec3::kOne;
            }
        }
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));

        meshes[i]->Submit();
    }
}

void SkinningScene::Submit_()
{
    Graphics& gfx = GetGraphics();

    // floor
    {
        m_pFloorTexture->Bind(eShader::PixelShader, kAlbedoTextureSlot);

        CBEntity entity   = {};
        entity.world      = Mtx4::kIdentity;
        entity.bHasAlbedo = 1;
        entity.bSkinned   = 0;
        entity.uvScale    = { 14.f, 14.f };
        entity.tint       = { 0.7f, 0.7f, 0.75f };
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        m_pFloor->Submit();
    }

    DrawCharacter_(m_globals, CalcCharacterWorld_());

    for (const Npc& npc: m_npcs)
    {
        DrawCharacter_(npc.globals, MakeCharacterWorld_(npc.position, npc.yaw));
    }
}
