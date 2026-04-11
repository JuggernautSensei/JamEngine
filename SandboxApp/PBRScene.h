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
        Vec3   pos          = Numbers<Vec3>::k_zero;
        Vec3   color        = Numbers<Vec3>::k_one;
        Vec3   lookAt       = Numbers<Vec3>::k_zero;
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

    ProgramHandle m_ph            = k_nullHandle;
    ProgramHandle m_cubeMapPh     = k_nullHandle;
    ProgramHandle m_toneMappingPh = k_nullHandle;

    ConstantBufferHandle m_entityCbh  = k_nullHandle;
    ConstantBufferHandle m_cmrCbh     = k_nullHandle;
    ConstantBufferHandle m_lightCbh   = k_nullHandle;
    ConstantBufferHandle m_cubeMapCbh = k_nullHandle;

    FrameBufferHandle m_fbh    = k_nullHandle;
    TextureHandle     m_fbTexh = k_nullHandle;   // not owned
    Viewport          m_vp     = {};

    VertexBufferHandle m_fullTriVbh = k_nullHandle;
    IndexBufferHandle  m_fullTriIbh = k_nullHandle;

    VertexBufferHandle m_cubeVbh = k_nullHandle;
    IndexBufferHandle  m_cubeIbh = k_nullHandle;

    bool  m_bAnimate     = true;
    float m_dYawRad      = ToRad(30.f);
    float m_entityYRad   = 0.f;
    float m_entityYawRad = 0.f;

    Ref<Mesh>    m_pMesh                = nullptr;
    Ref<Texture> m_pAlbedo              = nullptr;
    Ref<Texture> m_pAoRoughnessMetallic = nullptr;
    Ref<Texture> m_pEmissive            = nullptr;
    Ref<Texture> m_pNormal              = nullptr;

    Ref<Texture> m_pIBLIrradiance  = nullptr;
    Ref<Texture> m_pIBLSpecular    = nullptr;
    Ref<Texture> m_pIBLEnvironment = nullptr;
    Ref<Texture> m_pBRDFLUT        = nullptr;

    FlyingCamera m_camera = {};

    Array<Light, 4> m_lights = {};

    eCubeMap m_cubeMapType = eCubeMap::Irradiance;
};
