#include "pch.h"

#include "DemoScene.h"

DemoScene::DemoScene(const std::string_view& _name)
    : Scene(_name)
{
}

void DemoScene::OnAttach()
{
    m_dispatcher.AddListener<SwapChainResourceReleaseEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(DemoScene::OnSwapChainResourceReleaseEvent_));
    m_dispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(DemoScene::OnWindowResizeEvent_));
}

void DemoScene::OnEnter()
{
    Application&  app    = GetApplication();
    const Window& window = app.GetWindow();
    auto [width, height] = window.GetWindowSize();
    CreateScreenDependentResources_(width, height);

    // gemetry buffer
    {
        Vertex2 vertices[] = {
            Vertex2 {  .position = { 0.5f, 0.f },  .uv0 = { 0.f, 0.f } },
            Vertex2 { .position = { -0.5f, 0.f },  .uv0 = { 1.f, 0.f } },
            Vertex2 {  .position = { 0.f, 0.5f }, .uv0 = { 0.5f, 1.f } },
        };

        Index indices[] = {
            0, 1, 2
        };

        m_vertexBuffer = VertexBuffer::Create(eResourceAccess::Immutable, Vertex2::s_staticType, std::size(vertices), vertices);
        m_indexBuffer  = IndexBuffer::Create(eResourceAccess::Immutable, std::size(indices), indices);
    }

    // shader
    {
        constexpr const char* k_shader = R"(
            struct VS_INPUT
            {
                float2 position : POSITION;
                float2 uv0 : TEXCOORD0;
                float3 color : COLOR0;
            };

            struct PS_INPUT
            {
                    float4 position : SV_POSITION;
                    float3 color : COLOR0;
            };

            PS_INPUT VSMain(VS_INPUT input)
            {
                PS_INPUT output;
                output.position = float4(input.position, 0.0f, 1.0f);
                output.color = input.color;
                return output;
            }

            float4 PSMain(PS_INPUT input) : SV_TARGET
            {
                return float4(input.color, 1.0f);
            }
        )";

        ShaderCompiler vsCompiler;
        vsCompiler.CompileHLSL(k_shader, "VSMain", "vs_5_0");

        ShaderCompiler psCompiler;
        psCompiler.CompileHLSL(k_shader, "PSMain", "ps_5_0");

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> psBlob;
        vsCompiler.GetCompiledShader(vsBlob.GetAddressOf());
        psCompiler.GetCompiledShader(psBlob.GetAddressOf());
        m_shaderProgram = ShaderProgram::Create(vsBlob.Get(), psBlob.Get());
    }
}

void DemoScene::OnUpdate(float _deltaTime)
{
}

void DemoScene::OnRender()
{
    m_viewport.Bind();
    m_backBufferTex.BindAsRenderTarget();
    m_backBufferTex.ClearRenderTarget(&k_colorBlack.x);

    m_shaderProgram.Bind();
    m_vertexBuffer.Bind();
    m_indexBuffer.Bind();

    Renderer::SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Renderer::DrawIndices(m_indexBuffer.GetIndexCount(), 0, 0);
}

void DemoScene::OnRenderUI()
{
    ImGui::Begin("hello world!");

    ImGui::Text("This is a demo scene.");
    const TickTimer& timer = GetApplication().GetTimer();
    ImGui::Text("frame rate: %.4f", 1.f / timer.GetDeltaSec());
    ImGui::Text("delta sec: %.4f", timer.GetDeltaSec());

    ImGui::End();
}

void DemoScene::OnEvent(Event& _eventRef)
{
    m_dispatcher.Dispatch(_eventRef);
}

void DemoScene::OnSwapChainResourceReleaseEvent_(MAYBE_UNUSED const SwapChainResourceReleaseEvent& _event)
{
    m_backBufferTex.Reset();
}

void DemoScene::OnWindowResizeEvent_(const WindowResizeEvent& _event)
{
    UInt32 width  = _event.GetWidth();
    UInt32 height = _event.GetHeight();
    CreateScreenDependentResources_(width, height);
}

void DemoScene::CreateScreenDependentResources_(const UInt32 _width, const UInt32 _height)
{
    m_viewport = Viewport(0, 0, static_cast<float>(_width), static_cast<float>(_height));

    m_backBufferTex = Renderer::GetBackBufferTexture();
    m_backBufferTex.AttachRTV();
}