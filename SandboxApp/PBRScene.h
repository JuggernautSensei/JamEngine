#pragma once
#include "FlyingCamera.h"

class PBRScene : public Scene
{
    JAM_SCENE_BODY(PBRScene);

    enum class eLight : UInt32
    {
        Directional = 0,
        Point       = 1,
        Spot        = 2,
    };

    enum class eCubeMap
    {
        None,
        Irradiance,
        Specular,
        Environment
    };

    struct Light
    {
        Vec3   pos          = Vec3::kZero;
        Vec3   color        = Vec3::kOne;
        Vec3   lookAt       = Vec3::kZero;
        float  fallOffStart = 5.f;
        float  fallOffEnd   = 10.f;
        float  innerConeRad = ToRad(15.f);
        float  outerConeRad = ToRad(30.f);
        bool   bEnabled     = false;
        eLight type         = eLight::Point;
    };

public:
    PBRScene();
    ~PBRScene() noexcept override;

    PBRScene(const PBRScene&)                = delete;
    PBRScene& operator=(const PBRScene&)     = delete;
    PBRScene(PBRScene&&) noexcept            = delete;
    PBRScene& operator=(PBRScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    ProgramHandle m_ph            = kInvalidHandle;
    ProgramHandle m_cubeMapPh     = kInvalidHandle;
    ProgramHandle m_toneMappingPh = kInvalidHandle;

    ConstantBufferHandle m_entityCbh  = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh     = kInvalidHandle;
    ConstantBufferHandle m_lightCbh   = kInvalidHandle;
    ConstantBufferHandle m_cubeMapCbh = kInvalidHandle;

    FrameBufferHandle m_fbh    = kInvalidHandle;
    TextureHandle     m_colorTexh = kInvalidHandle;   // not owned

    VertexBufferHandle m_fullTriVbh = kInvalidHandle;
    IndexBufferHandle  m_fullTriIbh = kInvalidHandle;

    VertexBufferHandle m_cubeVbh = kInvalidHandle;
    IndexBufferHandle  m_cubeIbh = kInvalidHandle;

    bool  m_bAnimate = true;
    float m_dRot     = ToRad(45.f);
    float m_rot      = 0.f;

    Ref<Mesh>    m_pMesh                = nullptr;
    Ref<Texture> m_pAlbedo              = nullptr;
    Ref<Texture> m_pAoRoughnessMetallic = nullptr;
    Ref<Texture> m_pEmissive            = nullptr;
    Ref<Texture> m_pNormal              = nullptr;

    Ref<Texture> m_pIBLIrradiance  = nullptr;
    Ref<Texture> m_pIBLSpecular    = nullptr;
    Ref<Texture> m_pIBLEnvironment = nullptr;
    Ref<Texture> m_pBRDFLUT        = nullptr;

    Array<Light, 4> m_lights      = {};
    eCubeMap        m_cubeMapType = eCubeMap::Irradiance;
    FlyingCamera    m_camera      = {};
};
