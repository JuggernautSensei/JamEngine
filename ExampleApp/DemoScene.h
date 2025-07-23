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

    void OnAttach() override;
    void OnEnter() override;
    void OnUpdate(float _deltaTime) override;
    void OnRender() override;
    void OnRenderUI() override;
    void OnEvent(Event& _eventRef) override;

private:
    void OnSwapChainResourceReleaseEvent_(const SwapChainResourceReleaseEvent& _event);
    void OnWindowResizeEvent_(const WindowResizeEvent& _event);
    void CreateScreenDependentResources_(UInt32 _width, UInt32 _height);

    EventDispatcher m_dispatcher;

    Viewport  m_viewport;
    Texture2D m_backBufferTex;

    VertexBuffer   m_vertexBuffer;
    IndexBuffer    m_indexBuffer;
    ShaderProgram  m_shaderProgram;
    ConstantBuffer m_cbuffer;
};
