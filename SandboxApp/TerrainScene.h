#pragma once
#include "FlyingCamera.h"

class TerrainScene : public Scene
{
    JAM_SCENE_BODY(TerrainScene);

    struct NoiseParams
    {
        Int32 seed        = 1337;
        Int32 octaves     = 6;
        float frequency   = 2.f;
        float persistence = 0.5f;
        float lacunarity  = 2.f;
        float exponent    = 1.3f;
    };

    enum class eDebugView : Int32
    {
        None,
        Height,
        Normal,
        Weights,
    };

public:
    TerrainScene();
    ~TerrainScene() noexcept override;

    TerrainScene(const TerrainScene&)                = delete;
    TerrainScene& operator=(const TerrainScene&)     = delete;
    TerrainScene(TerrainScene&&) noexcept            = delete;
    TerrainScene& operator=(TerrainScene&&) noexcept = delete;

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;

    void InitTerrain_();
    void InitRenderGraph_();

    void GenerateHeights_();
    void UploadHeights_() const;

    void Submit_() const;

    RenderGraph          m_graph      = {};
    ConstantBufferHandle m_terrainCbh = kInvalidHandle;
    ConstantBufferHandle m_cmrCbh     = kInvalidHandle;

    // terrain
    VertexBufferHandle m_vbh        = kInvalidHandle;
    IndexBufferHandle  m_ibh        = kInvalidHandle;
    TextureHandle      m_heightTexh = kInvalidHandle;
    Vector<float>      m_heights;
    Ref<Texture>       m_pGrass = nullptr;
    Ref<Texture>       m_pRock  = nullptr;
    Ref<Texture>       m_pSnow  = nullptr;
    Ref<Texture>       m_pDirt  = nullptr;

    // camera
    FlyingCamera m_camera;

    // ui
    NoiseParams m_noise        = {};
    float       m_heightScale  = 240.f;
    float       m_tileSize     = 12.f;
    float       m_snowHeight   = 0.6f;
    float       m_snowBlend    = 0.12f;
    float       m_dirtHeight   = 0.2f;
    float       m_rockSlopeMin = 0.06f;
    float       m_rockSlopeMax = 0.18f;
    float       m_lightYaw     = ToRad(40.f);
    float       m_lightPitch   = ToRad(50.f);
    eDebugView  m_debugView    = eDebugView::None;
    bool        m_bWireframe   = false;
};
