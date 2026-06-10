#pragma once
#include "FlyingCamera.h"

class SimpleScene : public Scene
{
    JAM_SCENE_BODY(SimpleScene);

public:
    SimpleScene();
    ~SimpleScene() noexcept override;

    SimpleScene(const SimpleScene&)                = delete;
    SimpleScene& operator=(const SimpleScene&)     = delete;
    SimpleScene(SimpleScene&&) noexcept            = delete;
    SimpleScene& operator=(SimpleScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMeshes_();
    void InitRenderGraph_();
    void Submit_() const;

    RenderGraph          m_graph     = {};
    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    // entity
    Ref<Mesh> m_pCube     = nullptr;
    Transform m_cubeTrans = {};

    Ref<Mesh> m_pSphere     = nullptr;
    Transform m_sphereTrans = {};

    Ref<Mesh> m_pBunny     = nullptr;
    Transform m_bunnyTrans = {};

    bool  m_bAnimate = true;
    float m_dMove    = 2.f;
    float m_dRot     = ToRad(45.f);
    float m_counter  = 0.f;

    // camera
    FlyingCamera m_camera;

    // light
    Vec3 m_lightPos    = { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt = { 0.f, 0.f, 0.f };
    Vec3 m_lightCol    = { 1.f, 1.f, 1.f };
    Vec3 m_ambientCol  = { 0.1f, 0.1f, 0.1f };

    // material
    Vec3 m_baseCol = { 1.f, 1.f, 1.f };
};
