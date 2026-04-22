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
    void OnEvent(Event& _event) override;
    void OnRender(const TextureRef& _renderTarget, const Camera& _camera, const Transform& _cameraTransform)  override;

private:
    void OnWindowResizeEvent_(const WindowResizeEvent& _event);
    void OnWindowMaximizeEvent_(const WindowMaximizeEvent& _event);
    void CreateScreenDependentResources_(Int32 _width, Int32 _height);

private:
    EventDispatcher m_dispatcher;

    // frame resources
    TextureRef m_hdrTexture;
    TextureRef m_depthTexture;
    Viewport  m_viewport;

    TextureRef m_gBufferNormalTexture;
    TextureRef m_gBufferAlbedoRoughnessTexture;
    TextureRef m_gBufferMetallicAOTexture;
    TextureRef m_gBufferEmissionTexture;

    // post process
    PostProcess    m_postProcess;
    CB_POSTPROCESS m_cbPostProcess;

    // shaderHandles
    Program m_gBufferShader;

    // samplers
    SamplerState m_samplerPointerClamp;
    SamplerState m_samplerPointerWrap;
    SamplerState m_samplerLinearClamp;
    SamplerState m_samplerLinearWrap;
    SamplerState m_samplerLinearAnisotropic4Wrap;
    SamplerState m_samplerShadowComparisonLinearWrap;
};
