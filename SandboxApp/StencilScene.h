#pragma once
#include "FlyingCamera.h"

class StencilScene : public Scene
{
    JAM_SCENE_BODY(StencilScene);

    enum eObject : Int32
    {
        Bunny,
        Sphere,
        Cube,
        Cone,
        Cylinder,
        Ring,
        Count,
    };

    struct DrawParams
    {
        Vec3  color        = Vec3::kOne;
        float alpha        = 1.f;
        bool  bReflect     = false;
        float outlineWidth = 0.f;
    };

public:
    StencilScene();
    ~StencilScene() noexcept override;

    StencilScene(const StencilScene&)                = delete;
    StencilScene& operator=(const StencilScene&)     = delete;
    StencilScene(StencilScene&&) noexcept            = delete;
    StencilScene& operator=(StencilScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMeshes_();
    void InitRenderGraph_();

    void DrawObject_(eObject _object, const DrawParams& _params) const;
    void DrawFloor_(const DrawParams& _params) const;

    void DrawGroup_(bool _bSelected, bool _bReflect) const;
    void DrawOutline_(bool _bReflect) const;

    void SubmitOpaque_() const;
    void SubmitSelected_() const;
    void SubmitMirrorMask_() const;
    void SubmitReflectedOpaque_() const;
    void SubmitReflectedSelected_() const;
    void SubmitReflectedOutline_() const;
    void SubmitFloor_() const;
    void SubmitOutline_() const;

    [[nodiscard]] bool IsSelected_(eObject _object) const;

    RenderGraph          m_graph     = {};
    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    // entity
    Ref<Mesh> m_pObjects[eObject::Count]   = {};
    Transform m_transforms[eObject::Count] = {};
    Vec3      m_colors[eObject::Count]     = {};
    Ref<Mesh> m_pFloor                     = nullptr;

    // camera
    FlyingCamera m_camera;

    // light
    Vec3 m_lightPos    = { 3.f, 8.f, -5.f };
    Vec3 m_lightLookAt = { 0.f, 0.f, 0.f };
    Vec3 m_lightCol    = { 1.f, 1.f, 1.f };
    Vec3 m_ambientCol  = { 0.15f, 0.15f, 0.15f };

    // ui
    bool  m_bAnimate                 = true;
    bool  m_bOutline[eObject::Count] = { true, true, false, false, false, true };   // bunny, sphere, ring
    float m_outlineWidth             = 0.05f;
    Vec3  m_outlineColor             = { 1.f, 0.6f, 0.1f };
    bool  m_bMirror                  = true;
    float m_mirrorReflectivity       = 0.6f;
    Vec3  m_floorColor               = { 0.25f, 0.3f, 0.4f };
    float m_counter                  = 0.f;
};
