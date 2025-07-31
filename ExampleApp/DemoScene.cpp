#include "pch.h"

#include "DemoScene.h"

DemoScene::DemoScene(const std::string_view& _name)
    : Scene(_name)
{
    m_dispatcher.AddListener<BackBufferCleanupEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(DemoScene::OnSwapChainResourceReleaseEvent_));
    m_dispatcher.AddListener<WindowResizeEvent>(JAM_ADD_LISTENER_MEMBER_FUNCTION(DemoScene::OnWindowResizeEvent_));
}

void DemoScene::OnEnter()
{
    Application&  app    = GetApplication();
    const Window& window = app.GetWindow();
    auto [width, height] = window.GetWindowSize();
    CreateScreenDependentResources_(width, height);

    m_gBufferShader = ShaderCollection::PBRGBufferShader();
}

void DemoScene::OnUpdate(float _deltaTime)
{
}

void DemoScene::OnRender()
{
    // bind sampler states

    // update camera

    // clear gbuffer textures
    m_gBufferNormalTexture.ClearRenderTarget(k_pColorZero);
    m_gBufferAlbedoRoughnessTexture.ClearRenderTarget(k_pColorZero);
    m_gBufferMetallicAOTexture.ClearRenderTarget(k_pColorZero);
    m_gBufferEmissionTexture.ClearRenderTarget(k_pColorZero);
    m_depthTexture.ClearDepthStencil(true, false, 1.f, 0);

    // bind gbuffer textures as render targets
    ID3D11RenderTargetView* rtvArray[] = {
        m_gBufferNormalTexture.GetRTV(),
        m_gBufferAlbedoRoughnessTexture.GetRTV(),
        m_gBufferMetallicAOTexture.GetRTV(),
        m_gBufferEmissionTexture.GetRTV()
    };
    Renderer::BindRenderTargetViews(rtvArray, m_depthTexture.GetDSV());

    // bind viewport
    m_viewport.Bind();

    // apply post-processing
    m_postProcess.Render(m_hdrTexture);
}

void DemoScene::OnRenderUI()
{
    ImGui::Begin("hello world!");

    ImGui::Text("This is a demo scene.");
    const TickTimer& timer = GetApplication().GetTimer();
    ImGui::Text("frame rate: %.4f hz", 1.f / timer.GetDeltaSec());
    ImGui::Text("delta sec: %.4f sec", timer.GetDeltaSec());

    ImGui::End();
}

void DemoScene::OnEvent(Event& _eventRef)
{
    m_dispatcher.Dispatch(_eventRef);
}

void DemoScene::OnSwapChainResourceReleaseEvent_(MAYBE_UNUSED const BackBufferCleanupEvent& _event)
{
    m_backBufferTexture.Reset();
}

void DemoScene::OnWindowResizeEvent_(const WindowResizeEvent& _event)
{
    Int32 width  = _event.GetWidth();
    Int32 height = _event.GetHeight();
    CreateScreenDependentResources_(width, height);
}

void DemoScene::CreateScreenDependentResources_(const Int32 _width, const Int32 _height)
{
    // viewport
    m_viewport = { 0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height) };

    // back buffer
    m_backBufferTexture = Renderer::GetBackBufferTexture();
    m_backBufferTexture.AttachRTV();

    // depth buffer
    m_depthTexture.Initialize(_width, _height, DXGI_FORMAT_R24G8_TYPELESS, eResourceAccess::GPUWriteable, eViewFlags_DepthStencil | eViewFlags_ShaderResource);
    m_depthTexture.AttachDSV(DXGI_FORMAT_D24_UNORM_S8_UINT);
    m_depthTexture.AttachSRV(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

    // hdr back buffer
    m_hdrTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_hdrTexture.AttachRTV();
    m_hdrTexture.AttachSRV();

    // defered rendering backbuffer
    m_gBufferNormalTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_gBufferNormalTexture.AttachRTV();
    m_gBufferNormalTexture.AttachSRV();

    m_gBufferAlbedoRoughnessTexture.Initialize(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_gBufferAlbedoRoughnessTexture.AttachRTV();
    m_gBufferAlbedoRoughnessTexture.AttachSRV();

    m_gBufferMetallicAOTexture.Initialize(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_gBufferMetallicAOTexture.AttachRTV();
    m_gBufferMetallicAOTexture.AttachSRV();

    m_gBufferEmissionTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eResourceAccess::GPUWriteable, eViewFlags_ShaderResource | eViewFlags_RenderTarget);
    m_gBufferEmissionTexture.AttachRTV();
    m_gBufferEmissionTexture.AttachSRV();

    // post-process
    PostProcessBuilder builder;
    builder
        .AddBloomFilter(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, 4, m_hdrTexture)
        .AddToneMappingFilter(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eToneMappingFilterType::Linear)
        .AddFXAAFilter(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eFXAAQuality::High);
    m_postProcess = builder.Build(m_backBufferTexture);
}