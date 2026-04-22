#pragma once

class InstancingScene : public Scene
{
    JAM_SCENE_BODY(InstancingScene);

public:
    InstancingScene();
    ~InstancingScene() noexcept override;

    InstancingScene(const InstancingScene&)                = delete;
    InstancingScene& operator=(const InstancingScene&)     = delete;
    InstancingScene(InstancingScene&&) noexcept            = delete;
    InstancingScene& operator=(InstancingScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    ProgramHandle m_ph     = kInvalidHandle;
    ProgramHandle m_instPh = kInvalidHandle;

    ConstantBufferHandle m_entityNoInstancingCbh = kInvalidHandle;
    ConstantBufferHandle m_entityInstancingCbh   = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh                = kInvalidHandle;
    ConstantBufferHandle m_lightCbh              = kInvalidHandle;

    VertexBufferHandle                  m_instbh                  = kInvalidHandle;
    bool                                m_bDoInstancing           = true;
    Int32                               m_numLastRenderedEntities = 0;

    FrameBufferHandle m_fbh    = {};
    TextureHandle     m_fbTexh = {};   // not owned
    Viewport          m_vp     = {};

    bool  m_bAnimate     = true;
    float m_dMoveRad     = ToRad(90.f);
    float m_dYawRad      = ToRad(90.f);
    float m_entityYRad   = 0.f;
    float m_entityYawRad = 0.f;

    Int32 m_numEntitiesPerRow = 10;
    Int32 m_numRows           = 10;

    Ref<Mesh> m_pMesh = nullptr;

    Camera    m_camera          = {};
    Transform m_cameraTransform = {};

    Vec3 m_lightPos     = Vec3 { 0.f, 5.f, -5.f };
    Vec3 m_lightLookAt  = Vec3 { 0.f, 0.f, 0.f };
    Vec3 m_lightColor   = Vec3 { 1.f, 1.f, 1.f };
    Vec3 m_ambientColor = Vec3 { 0.1f, 0.1f, 0.1f };
        
    Vec3  m_baseColor = Vec3 { 1.f, 1.f, 1.f };
    float m_shiness   = 32.f;
};
