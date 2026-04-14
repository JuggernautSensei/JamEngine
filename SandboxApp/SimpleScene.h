#pragma once

class SimpleScene : public Scene
{
    JAM_SCENE_BODY(SimpleScene);

    enum class eMeshType
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

    ProgramHandle m_ph = k_invalidHandle;

    ConstantBufferHandle m_entityCbh = k_invalidHandle;
    ConstantBufferHandle m_cmrCbh    = k_invalidHandle;
    ConstantBufferHandle m_lightCbh  = k_invalidHandle;

    FrameBufferHandle m_fbh    = {};
    TextureHandle     m_fbTexh = {};   // not owned
    Viewport          m_vp     = {};

    bool  m_bAnimate     = true;
    float m_dMoveRad     = ToRad(90.f);
    float m_dYawRad      = ToRad(90.f);
    float m_entityYRad   = 0.f;
    float m_entityYawRad = 0.f;
    Int32 m_numEntities  = 1;

    EnumArray<eMeshType, Ref<Mesh>> m_pMesh      = {};
    eMeshType                       m_activeType = eMeshType::Cube;

    Camera    m_camera          = {};
    Transform m_cameraTransform = {};

    Vec3 m_lightPos     = Vec3 { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt  = Vec3 { 0.f, 0.f, 0.f };
    Vec3 m_lightColor   = Vec3 { 1.f, 1.f, 1.f };
    Vec3 m_ambientColor = Vec3 { 0.1f, 0.1f, 0.1f };

    Vec3  m_baseColor = Vec3 { 1.f, 1.f, 1.f };
    float m_shiness   = 32.f;
};
