#pragma once
#include "FlyingCamera.h"

class SimpleScene : public Scene
{
    JAM_SCENE_BODY(SimpleScene);

    enum class eMesh
    {
        Cube,
        Sphere,
        Bunny,
    };

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

    ProgramHandle m_ph = kInvalidHandle;

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    FrameBufferHandle m_fbh    = {};
    TextureHandle     m_fbTexh = {};   // not owned
    Viewport          m_vp     = {};

    bool  m_bAnimate     = true;
    float m_dMoveRad     = ToRad(90.f);
    float m_dYawRad      = ToRad(90.f);
    float m_entityYRad   = 0.f;
    float m_entityYawRad = 0.f;
    Int32 m_numEntities  = 1;

    EnumArray<eMesh, Ref<Mesh>> m_pMesh      = {};
    eMesh                       m_activeMesh = eMesh::Cube;

    // camera
    FlyingCamera m_camera;

    // light
    Vec3 m_lightPos    = { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt = { 0.f, 0.f, 0.f };
    Vec3 m_lightCol    = { 1.f, 1.f, 1.f };
    Vec3 m_ambientCol  = { 0.1f, 0.1f, 0.1f };

    // material
    Vec3  m_baseCol = { 1.f, 1.f, 1.f };
    float m_shiness = 32.f;
};
