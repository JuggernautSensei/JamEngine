#pragma once

class MultiVertexStreamScene : public Scene
{
    JAM_SCENE_BODY(MultiVertexStreamScene);

public:
    MultiVertexStreamScene();
    ~MultiVertexStreamScene() noexcept override;

    MultiVertexStreamScene(const MultiVertexStreamScene&)                = delete;
    MultiVertexStreamScene& operator=(const MultiVertexStreamScene&)     = delete;
    MultiVertexStreamScene(MultiVertexStreamScene&&) noexcept            = delete;
    MultiVertexStreamScene& operator=(MultiVertexStreamScene&&) noexcept = delete;

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

    VertexBufferHandle m_posNormalVbh = kInvalidHandle;
    VertexBufferHandle m_colorVbh     = kInvalidHandle;
    IndexBufferHandle  m_ibh          = kInvalidHandle;

    Camera    m_camera          = {};
    Transform m_cameraTransform = {};

    Vec3 m_lightPos     = Vec3 { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt  = Vec3 { 0.f, 0.f, 0.f };
    Vec3 m_lightColor   = Vec3 { 1.f, 1.f, 1.f };
    Vec3 m_ambientColor = Vec3 { 0.1f, 0.1f, 0.1f };

    Vec3  m_baseColor = Vec3 { 1.f, 1.f, 1.f };
    float m_shiness   = 32.f;
};
