#pragma once
#include "FlyingCamera.h"

class ParticleScene : public Scene
{
    JAM_SCENE_BODY(ParticleScene);

    struct Particle
    {
        Vec3   position;
        float  age;
        Vec3   velocity;
        float  lifetime;
        Vec4   color;
        float  angle;
        float  angularVelocity;
        UInt32 seed;
        float  size;
    };
    static_assert(sizeof(Particle) == 64);

public:
    static constexpr Int32 kCapacity = 100000;

    ParticleScene();
    ~ParticleScene() noexcept override = default;

    ParticleScene(const ParticleScene&)                = delete;
    ParticleScene& operator=(const ParticleScene&)     = delete;
    ParticleScene(ParticleScene&&) noexcept            = delete;
    ParticleScene& operator=(ParticleScene&&) noexcept = delete;

    void RunDemo(Int32 _index);
    void SetParticleCount(Int32 _count);

    void OnUpdate(float _deltaTime) override;
    void OnRender(const RenderView& _view) override;
    void OnImGuiRender() override;

private:
    void OnResize(Int32 _width, Int32 _height) override;
    void InitRenderGraph_();
    void Simulate_() const;
    void SubmitBackground_() const;
    void SubmitParticles_() const;
    void Restart_();

    RenderGraph          m_graph;
    ConstantBufferHandle m_simulationCbh = kInvalidHandle;
    ConstantBufferHandle m_cameraCbh     = kInvalidHandle;
    StorageBufferHandle  m_drawSbh       = kInvalidHandle;
    FlyingCamera         m_camera;

    // ui
    Int32 m_effect        = 0;
    Int32 m_count         = 12000;
    Int32 m_previousCount = 0;
    Int32 m_lastDrawCount = -1;
    Int32 m_seed          = 1;
    bool  m_bPaused       = false;
    bool  m_bReset        = true;
    float m_deltaTime     = 0.f;
    float m_time          = 0.f;
    float m_wind          = 1.f;
    float m_size          = 1.f;
    float m_fallSpeed     = 1.f;
};
