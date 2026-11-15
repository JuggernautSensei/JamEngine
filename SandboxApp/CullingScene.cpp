#include "pch.h"
#include "CullingScene.h"

#include <bit>

#include "Gui.h"
#include "ImageFilterManager.h"
#include "RandomGenerator.h"
#include "Utilities.h"
#include "RenderStates.h"

#define SCENE_PATH(_filepath) "Resources/CullingScene/" _filepath

namespace
{

struct CBEntity
{
    static constexpr Int32 kSlot = 0;

    Mtx4  world;
    Vec3  color;
    float padding1;
};

struct CBCamera
{
    static constexpr Int32 kSlot = 1;

    Vec3  eye;
    float padding1;
    Mtx4  viewProj;
};

constexpr Int32 kMaxObjects       = 200000;
constexpr Int32 kMaxLineVertices  = 1 << 17;
constexpr float kSpawnHalfExtentX = 300.f;
constexpr float kSpawnHalfExtentY = 60.f;
constexpr float kSpawnHalfExtentZ = 300.f;
constexpr float kLooseFactor      = 1.5f;
constexpr float kMoveRadius       = 20.f;
constexpr Int32 kTopViewSize      = 300;
constexpr Int32 kTopViewMargin    = 10;
constexpr float kTopViewExtent    = 320.f;

void SetObjectPosition(
    auto&      _object,
    const Vec3 _position)
{
    _object.aabb   = Aabb { _position, Vec3 { _object.size * 0.5f } };
    _object.worldT = Mtx4::MakeSRT(Vec3 { _object.size }, Quat::kIdentity, _position).Transpose();
}

void SmoothStat(
    float&      _stat,
    const float _value)
{
    _stat += (_value - _stat) * 0.1f;
}

}   // namespace

CullingScene::CullingScene()
{
    InitObjects_();
    InitRenderGraph_();

    m_camera.camera.SetFarZ(450.f);
    m_camera.dMove = 60.f;
    m_camera.transform.SetTranslation(Vec3::kZero);
}

CullingScene::~CullingScene()
{
    JAM_DESTROY_VERTEX_BUFFER(m_lineVbh);
    JAM_DESTROY_VERTEX_BUFFER(m_pointVbh);
}

void CullingScene::OnUpdate(
    const float _deltaTime)
{
    m_camera.Update(_deltaTime);

    if (m_bMove)
    {
        m_counter += _deltaTime;
        MoveObjects_();
    }
}

void CullingScene::OnRender(
    const RenderView& _view)
{
    Graphics& gfx = GetGraphics();

    CBCamera camera;
    camera.eye      = m_camera.GetEyePos();
    camera.viewProj = m_camera.CalcViewProj().Transpose();
    gfx.UpdateBuffer(m_cmrCbh, AsMemoryView(camera));

    if (!m_bFreezeCullCamera)
    {
        m_cullViewProj = m_camera.CalcViewProj();
        m_cullEye      = m_camera.GetEyePos();
    }
    Cull_(Frustum { m_cullViewProj }.Normalized());

    // top view. +z가 화면 위쪽
    const Mtx4 topView = Mtx4::MakeLookTo(Vec3 { 0.f, 400.f, 0.f }, Vec3 { 0.f, -1.f, 0.f }, Vec3 { 0.f, 0.f, 1.f });
    const Mtx4 topProj = Mtx4::MakeOrtho(kTopViewExtent * 2.f, kTopViewExtent * 2.f, 1.f, 800.f);

    CBCamera topCamera;
    topCamera.eye      = Vec3 { 0.f, 400.f, 0.f };
    topCamera.viewProj = (topView * topProj).Transpose();
    gfx.UpdateBuffer(m_topCmrCbh, AsMemoryView(topCamera));

    m_viewport = _view.viewport;

    m_graph.Execute(_view.fbh, _view.viewport);
}

void CullingScene::OnImGuiRender()
{
    if (!BeginSceneDemoWindow("CullingScene"))
    {
        ImGui::End();
        return;
    }

    // culling
    ImGui::SeparatorText("Culling");
    Int32 mode = static_cast<Int32>(m_mode);
    ImGui::RadioButton("None", &mode, static_cast<Int32>(eCullMode::None));
    ImGui::SameLine();
    ImGui::RadioButton("Brute Force", &mode, static_cast<Int32>(eCullMode::BruteForce));
    ImGui::SameLine();
    ImGui::RadioButton("Octree", &mode, static_cast<Int32>(eCullMode::SpaceDivision));
    m_mode = static_cast<eCullMode>(mode);

    Int32 numObjects = m_numObjects;
    if (ImGui::SliderInt("Objects", &numObjects, 1000, kMaxObjects))
    {
        SetObjectCount_(numObjects);
    }

    ImGui::Checkbox("Move 10% Of Objects", &m_bMove);
    ImGui::Checkbox("Freeze Culling Camera", &m_bFreezeCullCamera);

    // debug
    ImGui::SeparatorText("Debug");
    ImGui::Checkbox("Show Top View", &m_bShowTopView);
    ImGui::Checkbox("Show Octree", &m_bShowOctree);
    ImGui::BeginDisabled(!m_bShowOctree);
    ImGui::SliderInt("Octree Depth", &m_octreeShowDepth, 0, 8);
    ImGui::EndDisabled();

    // stats
    ImGui::SeparatorText("Stats");
    const Int32 numVisible = static_cast<Int32>(m_visible.size());
    ImGui::Text("Visible: %d / %d (%.1f%%)", numVisible, m_numObjects, 100.f * static_cast<float>(numVisible) / static_cast<float>(m_numObjects));
    ImGui::Text("Cull: %.3f ms", m_cullMs);
    ImGui::Text("Submit (CPU): %.3f ms", m_submitMs);
    ImGui::Text("Octree Update: %.3f ms", m_octreeUpdateMs);
    ImGui::Text("Octree Build: %.3f ms", m_octreeBuildMs);

    ImGui::End();
}

void CullingScene::OnResize(
    const Int32 _width,
    const Int32 _height)
{
    m_camera.SetAspectRatioFromBackBuffer();
}

void CullingScene::InitObjects_()
{
    VertexLayout attribLayout = {};
    attribLayout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
    m_pCube = CreateCubeMesh(Vec3 { 0.5f }, attribLayout);

    m_objects.resize(kMaxObjects);
    for (Object& object: m_objects)
    {
        object.basePosition = MakeRandomVec3(-kSpawnHalfExtentX, kSpawnHalfExtentX, -kSpawnHalfExtentY, kSpawnHalfExtentY, -kSpawnHalfExtentZ, kSpawnHalfExtentZ);
        object.color        = MakeRandomVec3(0.3f, 1.f, 0.3f, 1.f, 0.3f, 1.f);
        object.size         = MakeRandomFloat(0.5f, 1.5f);
        object.phase        = MakeRandomFloat(0.f, k2Pi);
        SetObjectPosition(object, object.basePosition);
    }

    m_visible.reserve(kMaxObjects);
    m_visibleFlags.reserve(kMaxObjects);
    m_pointVertices.reserve(kMaxObjects);
    SetObjectCount_(m_numObjects);
}

void CullingScene::InitRenderGraph_()
{
    Graphics& gfx = GetGraphics();

    LoadProgram(m_graph, "P_Object", SCENE_PATH("CullingShader.hlsl"));
    LoadProgram(m_graph, "P_Line", SCENE_PATH("LineShader.hlsl"));

    m_graph.EmplaceConstantBuffer("CB_Entity", sizeof(CBEntity));
    m_graph.EmplaceConstantBuffer("CB_Camera", sizeof(CBCamera));
    m_graph.EmplaceConstantBuffer("CB_TopCamera", sizeof(CBCamera));
    m_graph.EmplaceTexture2D("T_SceneColor", 1.f, eTextureFormat::RGBA8_UNorm, false, 1, eTextureFlags::RenderTarget);
    m_graph.EmplaceTexture2D("T_SceneDepth", 1.f, eTextureFormat::D32_Float, false, 1, eTextureFlags::DepthStencil);

    AttachmentRG attchments[2] = {};
    attchments[0].name         = "T_SceneColor";
    attchments[1].name         = "T_SceneDepth";
    m_graph.EmplaceFrameBuffer("FB_Scene", attchments);

    m_entityCbh = m_graph.GetConstantBuffer("CB_Entity");
    m_cmrCbh    = m_graph.GetConstantBuffer("CB_Camera");
    m_topCmrCbh = m_graph.GetConstantBuffer("CB_TopCamera");

    // line
    VertexLayout lineLayout = {};
    lineLayout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
    lineLayout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
    m_lineVbh = gfx.CreateDynamicVertexBuffer(kMaxLineVertices, lineLayout);
    m_lineVertices.reserve(kMaxLineVertices);
    m_pointVbh = gfx.CreateDynamicVertexBuffer(kMaxObjects, lineLayout);

    // 오브젝트 패스
    m_graph.AddRenderPass("Pass_Object")
        .SetProgram("P_Object")
        .SetFrameBuffer("FB_Scene")
        .SetRenderTargetClearCommand(RGBA::FromLinear(0.02f, 0.03f, 0.05f, 1.f), 0)
        .SetDepthClearCommand(1.f)
        .SetConstantBuffer("CB_Entity", eShader::VertexShader, 0)
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetConstantBuffer("CB_Entity", eShader::PixelShader, 0)
        .SetRenderFlags(kBackCullDepthTest)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&CullingScene::SubmitScene_>(this));

    // 디버그 라인 패스
    m_graph.AddRenderPass("Pass_Debug")
        .SetProgram("P_Line")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_Camera", eShader::VertexShader, 1)
        .SetRenderFlags(eRenderFlags::PrimitiveTopology_LineList | eRenderFlags::DepthFunc_LessEqual)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&CullingScene::SubmitDebug_>(this));

    // 탑뷰 배경 패스
    m_graph.AddRenderPass("Pass_TopViewBackground")
        .SetProgram("P_Line")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_TopCamera", eShader::VertexShader, 1)
        .SetRenderFlags(eRenderFlags::PrimitiveTopology_TriangleList)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&CullingScene::SubmitTopViewBackground_>(this));

    // 탑뷰 오브젝트 패스
    m_graph.AddRenderPass("Pass_TopViewPoints")
        .SetProgram("P_Line")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_TopCamera", eShader::VertexShader, 1)
        .SetRenderFlags(eRenderFlags::PrimitiveTopology_PointList)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&CullingScene::SubmitTopViewPoints_>(this));

    // 탑뷰 라인 패스
    m_graph.AddRenderPass("Pass_TopViewLines")
        .SetProgram("P_Line")
        .SetFrameBuffer("FB_Scene")
        .SetConstantBuffer("CB_TopCamera", eShader::VertexShader, 1)
        .SetRenderFlags(eRenderFlags::PrimitiveTopology_LineList)
        .SetBlendFlags(kOpaqueBlend, 0)
        .SetExecutor(Bind<&CullingScene::SubmitTopViewLines_>(this));

    // blit
    ImageFilterManager& ifm = GetImageFilterManager();

    ImageFilterInfo blitInfo = {};
    blitInfo.filter          = eImageFilter::Blit;
    blitInfo.output          = eImageFilterOutput::Final;
    blitInfo.input0Texture   = "T_SceneColor";
    ifm.AddImageFilter(m_graph, "Pass_Blit", blitInfo);
}

void CullingScene::SetObjectCount_(
    const Int32 _count)
{
    m_numObjects = Clamp(_count, 1, kMaxObjects);

    for (Int32 i = 0; i < m_numObjects; ++i)
    {
        if (!m_objects[i].entity)
        {
            m_objects[i].entity = CreateEntity();
        }
    }

    m_bOctreeDirty = true;
}

void CullingScene::MoveObjects_()
{
    const Int32 numMoving = m_numObjects / 10;
    const bool  bUpdate   = m_mode == eCullMode::SpaceDivision && m_pOctree && !m_bOctreeDirty;

    Timer timer;
    timer.Restart();
    for (Int32 i = 0; i < numMoving; ++i)
    {
        Object& object = m_objects[i];

        Vec3 position = object.basePosition;
        position.x += Sin(m_counter * 1.5f + object.phase) * kMoveRadius;
        position.z += Cos(m_counter * 1.5f + object.phase) * kMoveRadius;
        SetObjectPosition(object, position);

        if (bUpdate)
        {
            m_pOctree->Update(static_cast<OctreeItemHandle>(i), object.aabb);
        }
    }
    timer.Stop();

    if (bUpdate)
    {
        SmoothStat(m_octreeUpdateMs, timer.GetTotalFloat(eSIPrefix::Milli));
    }
    else
    {
        m_bOctreeDirty = true;
    }
}

void CullingScene::BuildOctree_()
{
    OctreeInfo info           = {};
    info.initBaseExtents      = Vec3 { 320.f };
    info.maxDepth             = 6;
    info.subdivisionThreshold = 32;
    info.looseFactor          = kLooseFactor;

    Timer timer;
    timer.Restart();
    m_pOctree = MakeScoped<OctreeT>(info);

    m_buildItems.clear();
    for (Int32 i = 0; i < m_numObjects; ++i)
    {
        m_buildItems.push_back({ m_objects[i].entity, m_objects[i].aabb, i });
    }
    m_pOctree->Build(m_buildItems);   // 핸들은 입력 인덱스와 같다
    timer.Stop();

    m_octreeBuildMs = timer.GetTotalFloat(eSIPrefix::Milli);
    m_bOctreeDirty  = false;
}

void CullingScene::Cull_(
    const Frustum& _frustum)
{
    if (m_mode == eCullMode::SpaceDivision && (m_bOctreeDirty || !m_pOctree))
    {
        BuildOctree_();
    }

    Timer timer;
    timer.Restart();

    m_visible.clear();
    switch (m_mode)
    {
        case eCullMode::None:
            for (Int32 i = 0; i < m_numObjects; ++i)
            {
                m_visible.push_back(i);
            }
            break;

        case eCullMode::BruteForce:
            for (Int32 i = 0; i < m_numObjects; ++i)
            {
                if (_frustum.Classify(m_objects[i].aabb) != eContainment::Outside)
                {
                    m_visible.push_back(i);
                }
            }
            break;

        case eCullMode::SpaceDivision:
        {
            // 트리 순서 결과를 비트맵으로 인덱스 오름차순 정렬 (m_objects 순차 읽기용)
            const Int32 numWords = (m_numObjects + 63) / 64;
            m_visibleBits.assign(numWords, 0);
            m_pOctree->QueryClassified(
                _frustum,
                [](const Frustum& _f, const Aabb& _aabb) { return _f.Classify(_aabb); },
                [this](Entity, const Int32& _index) { m_visibleBits[_index >> 6] |= 1ull << (_index & 63); });

            for (Int32 w = 0; w < numWords; ++w)
            {
                for (UInt64 bits = m_visibleBits[w]; bits != 0; bits &= bits - 1)
                {
                    m_visible.push_back(w * 64 + std::countr_zero(bits));
                }
            }
            break;
        }
    }

    timer.Stop();
    SmoothStat(m_cullMs, timer.GetTotalFloat(eSIPrefix::Milli));
}

void CullingScene::SubmitScene_()
{
    Graphics& gfx = GetGraphics();

    Timer timer;
    timer.Restart();
    for (const Int32 index: m_visible)
    {
        const Object& object = m_objects[index];

        CBEntity entity;
        entity.world = object.worldT;
        entity.color = object.color;
        gfx.UpdateBuffer(m_entityCbh, AsMemoryView(entity));
        m_pCube->Submit();
    }
    timer.Stop();

    SmoothStat(m_submitMs, timer.GetTotalFloat(eSIPrefix::Milli));
}

void CullingScene::SubmitDebug_()
{
    m_lineVertices.clear();

    if (m_bShowOctree && m_mode == eCullMode::SpaceDivision && m_pOctree && !m_bOctreeDirty)
    {
        const RGBA depthColors[] = {
            RGBA::FromLinear(1.f, 0.2f, 0.2f, 1.f),
            RGBA::FromLinear(1.f, 0.6f, 0.2f, 1.f),
            RGBA::FromLinear(0.9f, 0.9f, 0.2f, 1.f),
            RGBA::FromLinear(0.3f, 0.9f, 0.3f, 1.f),
            RGBA::FromLinear(0.2f, 0.8f, 0.9f, 1.f),
            RGBA::FromLinear(0.3f, 0.4f, 1.f, 1.f),
            RGBA::FromLinear(0.7f, 0.3f, 1.f, 1.f),
            RGBA::FromLinear(1.f, 0.3f, 0.8f, 1.f),
            RGBA::FromLinear(1.f, 1.f, 1.f, 1.f),
        };

        m_pOctree->TraverseNodes(
            [&](const Aabb& _looseAabb, const Int32 _depth, const Int32 _numItems, const bool _bLeaf) {
                if (_depth > m_octreeShowDepth || m_lineVertices.size() + 24 > kMaxLineVertices)
                {
                    return;
                }

                const Aabb tightAabb = { _looseAabb.center, _looseAabb.extents / kLooseFactor };
                PushBox_(tightAabb.CalcCorners(), depthColors[Min(_depth, 8)]);
            });
    }

    SubmitVertices_(m_lineVbh, m_lineVertices);
}

void CullingScene::SubmitTopViewBackground_()
{
    if (!m_bShowTopView)
    {
        return;
    }

    SetTopViewViewport_();

    const RGBA  color = RGBA::FromLinear(0.04f, 0.05f, 0.07f, 1.f);
    const float e     = kTopViewExtent;
    const Vec3  p0    = { -e, 0.f, -e };
    const Vec3  p1    = { -e, 0.f, e };
    const Vec3  p2    = { e, 0.f, e };
    const Vec3  p3    = { e, 0.f, -e };

    Vector<LineVertex> quad = {
        { p0, color },
        { p1, color },
        { p2, color },
        { p0, color },
        { p2, color },
        { p3, color }
    };
    SubmitVertices_(m_lineVbh, quad);
}

void CullingScene::SubmitTopViewPoints_()
{
    if (!m_bShowTopView)
    {
        return;
    }

    SetTopViewViewport_();

    m_visibleFlags.assign(m_numObjects, 0);
    for (const Int32 index: m_visible)
    {
        m_visibleFlags[index] = 1;
    }

    // 컬링된 점을 먼저 그려서 보이는 점이 위에 오게 한다
    const RGBA culledColor  = RGBA::FromLinear(0.25f, 0.25f, 0.3f, 1.f);
    const RGBA visibleColor = RGBA::FromLinear(0.3f, 1.f, 0.3f, 1.f);

    m_pointVertices.clear();
    for (Int32 i = 0; i < m_numObjects; ++i)
    {
        if (!m_visibleFlags[i])
        {
            m_pointVertices.push_back({ m_objects[i].aabb.center, culledColor });
        }
    }
    for (const Int32 index: m_visible)
    {
        m_pointVertices.push_back({ m_objects[index].aabb.center, visibleColor });
    }

    SubmitVertices_(m_pointVbh, m_pointVertices);
}

void CullingScene::SubmitTopViewLines_()
{
    if (!m_bShowTopView)
    {
        return;
    }

    SetTopViewViewport_();

    m_lineVertices.clear();

    // 스폰 영역
    const float e           = kTopViewExtent;
    const RGBA  borderColor = RGBA::FromLinear(0.5f, 0.5f, 0.55f, 1.f);
    PushLine_({ -e, 0.f, -e }, { -e, 0.f, e }, borderColor);
    PushLine_({ -e, 0.f, e }, { e, 0.f, e }, borderColor);
    PushLine_({ e, 0.f, e }, { e, 0.f, -e }, borderColor);
    PushLine_({ e, 0.f, -e }, { -e, 0.f, -e }, borderColor);

    // 컬링 카메라 프러스텀
    PushBox_(Frustum::CalcCorners(m_cullViewProj.Inverse()), RGBA::FromLinear(1.f, 0.9f, 0.1f, 1.f));

    // 카메라 위치. 노랑이 컬링 카메라, 흰색이 보는 카메라
    const auto pushCross = [this](const Vec3 _center, const RGBA _rgba) {
        constexpr float kSize = 8.f;
        PushLine_(_center - Vec3 { kSize, 0.f, 0.f }, _center + Vec3 { kSize, 0.f, 0.f }, _rgba);
        PushLine_(_center - Vec3 { 0.f, 0.f, kSize }, _center + Vec3 { 0.f, 0.f, kSize }, _rgba);
    };
    pushCross(m_cullEye, RGBA::FromLinear(1.f, 0.9f, 0.1f, 1.f));
    pushCross(m_camera.GetEyePos(), RGBA::FromLinear(1.f, 1.f, 1.f, 1.f));

    SubmitVertices_(m_lineVbh, m_lineVertices);
}

void CullingScene::SetTopViewViewport_() const
{
    const Int32 x = Max(static_cast<Int32>(m_viewport.width) - kTopViewSize - kTopViewMargin, 0);
    GetGraphics().SetViewport(static_cast<UInt16>(x), static_cast<UInt16>(kTopViewMargin), static_cast<UInt16>(kTopViewSize), static_cast<UInt16>(kTopViewSize));
}

void CullingScene::SubmitVertices_(
    const VertexBufferHandle  _vbh,
    const Vector<LineVertex>& _vertices) const
{
    if (_vertices.empty())
    {
        return;
    }

    Graphics& gfx = GetGraphics();

    const Int32 numVerts = static_cast<Int32>(_vertices.size());
    gfx.UpdateBuffer(_vbh, MemoryView { _vertices.data(), static_cast<SizeT>(_vertices.size() * sizeof(LineVertex)) });
    gfx.SetVertexBuffer(_vbh, 0, numVerts);
    gfx.SetIndexBuffer(kInvalidHandle);
    gfx.Submit();
}

void CullingScene::PushLine_(
    const Vec3 _a,
    const Vec3 _b,
    const RGBA _rgba)
{
    m_lineVertices.push_back({ _a, _rgba });
    m_lineVertices.push_back({ _b, _rgba });
}

void CullingScene::PushBox_(
    const Corners& _corners,
    const RGBA     _rgba)
{
    // 코너 인덱스는 x y z 비트 순서. 한 비트만 다른 쌍이 모서리
    constexpr Int32 kEdges[12][2] = {
        { 0, 1 },
        { 2, 3 },
        { 4, 5 },
        { 6, 7 },
        { 0, 2 },
        { 1, 3 },
        { 4, 6 },
        { 5, 7 },
        { 0, 4 },
        { 1, 5 },
        { 2, 6 },
        { 3, 7 },
    };

    for (const auto& edge: kEdges)
    {
        PushLine_(_corners[static_cast<eRegion>(edge[0])], _corners[static_cast<eRegion>(edge[1])], _rgba);
    }
}
