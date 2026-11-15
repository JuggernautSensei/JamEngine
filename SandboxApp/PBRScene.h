#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

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

    enum class eToneMapping : Int32
    {
        None        = 0,
        Linear      = 1,
        Exponential = 2,
        Reinhard    = 3,
        Uncharted2  = 4,
        Mgsv        = 5,
        Uchimura    = 6,
        Filmic      = 7,
        Aces        = 8,
        PbrNeutral  = 9,
        Flim        = 10,
        Agx         = 11,
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

    struct TextureSet
    {
        Ref<Texture> pAlbedo = nullptr;
        Ref<Texture> pArm    = nullptr;
        Ref<Texture> pNormal = nullptr;
    };

    struct SceneObject
    {
        Ref<Mesh> pMesh     = nullptr;
        Int32     texSet    = -1;   // -1: 상수 머테리얼
        Vec3      position  = Vec3::kZero;
        Vec3      euler     = Vec3::kZero;
        Vec3      albedo    = Vec3::kOne;
        float     roughness = 1.f;
        float     metallic  = 0.f;
        float     spinSpeed = 0.f;
        float     scale     = 1.f;
        Vec2      uvScale   = { 1.f, 1.f };
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
    void LoadEnvironment_(Int32 _index);
    void OnResize(Int32 _width, Int32 _height) override;
    void InitObjects_();
    void InitRenderGraph_();
    void SubmitCubeMap_() const;
    void SubmitPBR_();
    void UpdateToneMappingParams_() const;

    RenderGraph m_graph = {};

    ConstantBufferHandle m_entityCbh   = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh      = kInvalidHandle;
    ConstantBufferHandle m_lightCbh    = kInvalidHandle;
    ConstantBufferHandle m_cubeMapCbh  = kInvalidHandle;
    ConstantBufferHandle m_materialCbh = kInvalidHandle;

    VertexBufferHandle m_cubeVbh = kInvalidHandle;
    IndexBufferHandle  m_cubeIbh = kInvalidHandle;

    bool  m_bAnimate = true;
    float m_dRot     = ToRad(45.f);
    float m_rot      = 0.f;

    Vector<TextureSet>  m_textureSets = {};
    Vector<SceneObject> m_objects     = {};

    Ref<Texture> m_pIBLIrradiance  = nullptr;
    Ref<Texture> m_pIBLSpecular    = nullptr;
    Ref<Texture> m_pIBLEnvironment = nullptr;
    Ref<Texture> m_pBRDFLUT        = nullptr;

    eToneMapping m_tmType   = eToneMapping::Aces;
    float        m_exposure = -1.2f;
    float        m_gamma    = 2.2f;

    Array<Light, 4> m_lights           = {};
    eCubeMap        m_cubeMapType      = eCubeMap::Environment;
    Int32           m_environmentIndex = 0;
    Vec3            m_backgroundColor  = { 0.04f, 0.045f, 0.055f };
    float           m_lightTime        = 0.f;
    FlyingCamera    m_camera           = {};
};
