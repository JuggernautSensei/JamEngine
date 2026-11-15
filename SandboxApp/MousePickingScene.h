#pragma once
#include "FlyingCamera.h"

class MousePickingScene : public Scene
{
    JAM_SCENE_BODY(MousePickingScene);

    struct Object
    {
        Ref<Mesh> pMesh;
        Transform transform;
        Vec3      color;
    };

public:
    MousePickingScene();
    ~MousePickingScene() noexcept override = default;

    MousePickingScene(const MousePickingScene&)                = delete;
    MousePickingScene& operator=(const MousePickingScene&)     = delete;
    MousePickingScene(MousePickingScene&&) noexcept            = delete;
    MousePickingScene& operator=(MousePickingScene&&) noexcept = delete;

    void RequestPick(Vec2i _clientPoint);
    void SetShowIds(bool _bShowIds);

    void OnEvent(Event& _event) override;
    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;
    void OnMouseDown_(const MouseDownEvent& _event);
    void InitObjects_();
    void InitRenderGraph_();
    void SubmitObjects_() const;
    void SubmitComposite_();
    void ReadPick_();
    void DrawObject_(const Mesh& _mesh, const Mtx4& _world, Vec3 _color, Int32 _id) const;

    RenderGraph          m_graph;
    ConstantBufferHandle m_entityCbh    = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh    = kInvalidHandle;
    ConstantBufferHandle m_selectionCbh = kInvalidHandle;
    Vector<Object>       m_objects;
    Ref<Mesh>            m_pFloor;
    FlyingCamera         m_camera;
    Viewport             m_viewport;
    Vec2i                m_pickPoint      = {};
    Int32                m_selectedId     = 0;
    Int32                m_outlineWidth   = 2;
    bool                 m_bPickRequested = false;
    bool                 m_bShowIds       = false;
    bool                 m_bAnimate       = false;
    float                m_time           = 0.f;
};
