#pragma once
#include "FlyingCamera.h"

class SSRScene : public Scene
{
    JAM_SCENE_BODY(SSRScene);

    struct Object
    {
        Ref<Mesh>    pMesh        = nullptr;
        Ref<Texture> pTexture     = nullptr;
        Transform    transform    = {};
        Vec3         color        = Vec3::kOne;
        float        reflectivity = 0.f;
    };

public:
    SSRScene();
    ~SSRScene() noexcept override;

    SSRScene(const SSRScene&)                = delete;
    SSRScene& operator=(const SSRScene&)     = delete;
    SSRScene(SSRScene&&) noexcept            = delete;
    SSRScene& operator=(SSRScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitObjects_();
    void InitRenderGraph_();

    void SubmitGBuffer_() const;
    void SubmitScreenTriangle_() const;

    RenderGraph          m_graph        = {};
    ConstantBufferHandle m_entityCbh    = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightingCbh  = kInvalidHandle;
    ConstantBufferHandle m_ssrCbh       = kInvalidHandle;
    ConstantBufferHandle m_compositeCbh = kInvalidHandle;

    // object
    Vector<Object> m_objects;
    Int32          m_sphereIndex = 0;
    Int32          m_cubeIndex   = 0;

    // camera
    FlyingCamera m_camera;

    // ui
    bool  m_bAnimate          = true;
    bool  m_bUseSsr           = true;
    bool  m_bReflectionOnly   = false;
    bool  m_bSkyFallback      = true;
    float m_maxDistance       = 30.f;
    Int32 m_steps             = 1024;
    float m_thickness         = 0.3f;
    float m_edgeFade          = 0.15f;
    float m_strength          = 1.f;
    float m_floorReflectivity = 0.7f;
    float m_ambient           = 0.35f;
    float m_counter           = 0.f;
};
