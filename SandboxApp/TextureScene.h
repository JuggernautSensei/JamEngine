#pragma once
#include "FlyingCamera.h"
#include "../JamEngine/RenderGraph.h"

class TextureScene : public Scene
{
    JAM_SCENE_BODY(TextureScene);

    enum class eTextureMappingFlags : UInt32
    {
        None   = 0,
        Albedo = 1 << 0,
        Ao     = 1 << 1,
        Normal = 1 << 2
    };

    struct SurfaceMaterial
    {
        Ref<Texture> pAlbedo = nullptr;
        Ref<Texture> pAo     = nullptr;
        Ref<Texture> pNormal = nullptr;
    };

    struct SceneObject
    {
        Ref<Mesh> pMesh     = nullptr;
        Int32     material  = 0;
        Vec3      position  = Vec3::kZero;
        float     scale     = 1.f;
        Vec3      baseEuler = Vec3::kZero;
        float     spinSpeed = 0.f;
        Vec2      uvScale   = { 1.f, 1.f };
    };

public:
    TextureScene();
    ~TextureScene() noexcept override;

    TextureScene(const TextureScene&)                = delete;
    TextureScene& operator=(const TextureScene&)     = delete;
    TextureScene(TextureScene&&) noexcept            = delete;
    TextureScene& operator=(TextureScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitMaterials_();
    void InitObjects_();
    void InitRenderGraph_();
    void Submit_() const;

    RenderGraph m_graph = {};

    ConstantBufferHandle m_entityCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh    = kInvalidHandle;
    ConstantBufferHandle m_lightCbh  = kInvalidHandle;

    Vector<SurfaceMaterial> m_materials = {};
    Vector<SceneObject>     m_objects   = {};

    FlyingCamera m_camera = {};

    Flags<eTextureMappingFlags> m_textureMappingFlags = eTextureMappingFlags::Albedo | eTextureMappingFlags::Ao | eTextureMappingFlags::Normal;

    bool  m_bAnimateObjects = true;
    bool  m_bAnimateLight   = true;
    float m_time            = 0.f;
    float m_lightAngle      = 0.6f;
    float m_lightHeight     = 0.5f;

    Vec3 m_lightColor   = Vec3 { 1.25f, 1.2f, 1.1f };
    Vec3 m_ambientColor = Vec3 { 0.34f, 0.34f, 0.4f };
};
