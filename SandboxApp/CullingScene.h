#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/Octree.h"

class CullingScene : public Scene
{
    JAM_SCENE_BODY(CullingScene);

    enum class eCullMode : Int32
    {
        None,
        BruteForce,
        SpaceDivision,
    };

    struct Object
    {
        Vec3   basePosition = Vec3::kZero;
        Vec3   color        = Vec3::kOne;
        float  size         = 1.f;
        float  phase        = 0.f;
        Aabb   aabb         = {};
        Mtx4   worldT       = Mtx4::kIdentity;
        Entity entity       = Entity::kNull;
    };

    using OctreeT = Octree<Int32>;

    struct LineVertex
    {
        Vec3 point;
        RGBA rgba;
    };

public:
    CullingScene();
    ~CullingScene() noexcept override;

    CullingScene(const CullingScene&)                = delete;
    CullingScene& operator=(const CullingScene&)     = delete;
    CullingScene(CullingScene&&) noexcept            = delete;
    CullingScene& operator=(CullingScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitObjects_();
    void InitRenderGraph_();

    void SetObjectCount_(Int32 _count);
    void MoveObjects_();
    void BuildOctree_();
    void Cull_(const Frustum& _frustum);

    void SubmitScene_();
    void SubmitDebug_();
    void SubmitTopViewBackground_();
    void SubmitTopViewPoints_();
    void SubmitTopViewLines_();

    void SetTopViewViewport_() const;
    void SubmitVertices_(VertexBufferHandle _vbh, const Vector<LineVertex>& _vertices) const;

    void PushLine_(Vec3 _a, Vec3 _b, RGBA _rgba);
    void PushBox_(const Corners& _corners, RGBA _rgba);

    RenderGraph          m_graph     = {};
    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_topCmrCbh = kInvalidHandle;
    VertexBufferHandle   m_lineVbh   = kInvalidHandle;
    VertexBufferHandle   m_pointVbh  = kInvalidHandle;

    // entity
    Ref<Mesh>                  m_pCube = nullptr;
    Vector<Object>             m_objects;
    Vector<Int32>              m_visible;
    Vector<UInt64>             m_visibleBits;
    Vector<UInt8>              m_visibleFlags;
    Int32                      m_numObjects = 20000;
    Scoped<OctreeT>            m_pOctree    = nullptr;
    Vector<OctreeT::BuildItem> m_buildItems;
    bool                       m_bOctreeDirty = true;

    // camera
    FlyingCamera m_camera;
    Mtx4         m_cullViewProj      = Mtx4::kIdentity;
    Vec3         m_cullEye           = Vec3::kZero;
    Viewport     m_viewport          = {};
    bool         m_bFreezeCullCamera = false;

    // ui
    eCullMode m_mode            = eCullMode::SpaceDivision;
    bool      m_bMove           = false;
    bool      m_bShowTopView    = true;
    bool      m_bShowOctree     = false;
    Int32     m_octreeShowDepth = 4;
    float     m_counter         = 0.f;

    // stats
    float m_cullMs         = 0.f;
    float m_submitMs       = 0.f;
    float m_octreeUpdateMs = 0.f;
    float m_octreeBuildMs  = 0.f;

    // line
    Vector<LineVertex> m_lineVertices;
    Vector<LineVertex> m_pointVertices;
};
