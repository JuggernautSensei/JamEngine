#pragma once

class DemoScene : public Scene
{
public:
    explicit DemoScene(const std::string_view& _name);
    ~DemoScene() override = default;

    DemoScene(const DemoScene&)                = delete;
    DemoScene& operator=(const DemoScene&)     = delete;
    DemoScene(DemoScene&&) noexcept            = default;
    DemoScene& operator=(DemoScene&&) noexcept = default;

    void OnEnter() override;
    void OnUpdate(float _deltaTime) override;
    void OnRender() override;
    void OnRenderUI() override;
    void OnEvent(Event& _eventRef) override;

private:
    void OnSwapChainResourceReleaseEvent_(const BackBufferCleanupEvent& _event);
    void OnWindowResizeEvent_(const WindowResizeEvent& _event);
    void CreateScreenDependentResources_(Int32 _width, Int32 _height);

    EventDispatcher m_dispatcher;

    // frame resources
    Texture2D m_backBufferTexture;
    Texture2D m_hdrTexture;
    Texture2D m_depthTexture;
    Viewport  m_viewport;

    Texture2D m_gBufferNormalTexture;
    Texture2D m_gBufferAlbedoRoughnessTexture;
    Texture2D m_gBufferMetallicAOTexture;
    Texture2D m_gBufferEmissionTexture;

    // post process
    PostProcess    m_postProcess;
    CB_POSTPROCESS m_cbPostProcess;

    // shaders
    ShaderProgram m_gBufferShader;

    // samplers
    SamplerState m_samplerPointerClamp;
    SamplerState m_samplerPointerWrap;
    SamplerState m_samplerLinearClamp;
    SamplerState m_samplerLinearWrap;
    SamplerState m_samplerLinearAnisotropic4Wrap;
    SamplerState m_samplerShadowComparisonLinearWrap;

    // camera
    Entity    m_cameraEntity;
};
