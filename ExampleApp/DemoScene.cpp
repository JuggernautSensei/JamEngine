#include "pch.h"

#include "DemoScene.h"

DemoScene::DemoScene(const std::string_view& _name)
    : Scene(_name)
{
    m_dispatcher.AddListener<WindowResizeEvent>(JAM_BIND_MEMBER_LISTENER(DemoScene::OnWindowResizeEvent_));

    // shaderHandles
    m_gBufferShader = ShaderRegistry::PBRGBufferShader();

    // samplers
    m_samplerLinearWrap                 = StateRegistry::SamplerLinearWrap();
    m_samplerLinearClamp                = StateRegistry::SamplerLinearClamp();
    m_samplerPointerWrap                = StateRegistry::SamplerPointWrap();
    m_samplerPointerClamp               = StateRegistry::SamplerPointClamp();
    m_samplerLinearAnisotropic4Wrap     = StateRegistry::SamplerLinearAnisotropic4Wrap();
    m_samplerShadowComparisonLinearWrap = StateRegistry::SamplerShadowComparisonLinearWrap();
}

void DemoScene::OnEnter()
{
    const Window& window = Application::GetInstance().GetWindow();
    auto [width, height] = window.GetWindowSize();
    CreateScreenDependentResources_(width, height);

    // post-process
    {
        m_cbPostProcess.cb_postProcessBlurRadius      = 0.04f;
        m_cbPostProcess.cb_postProcessCombineStrength = 1.f;
        m_cbPostProcess.cb_postProcessExposure        = 1.f;
        m_cbPostProcess.cb_postProcessGamma           = 2.2f;
    }
}

void DemoScene::OnUpdate(float _deltaTime)
{
}

void DemoScene::OnRender(const TextureRef& _renderTarget, const Camera& _camera, const Transform& _cameraTransform)
{
    {
        // bind sampler states
        ID3D11SamplerState* samplers[] = {
            m_samplerPointerClamp.Get(),
            m_samplerPointerWrap.Get(),
            m_samplerLinearClamp.Get(),
            m_samplerLinearWrap.Get(),
            m_samplerLinearAnisotropic4Wrap.Get(),
            m_samplerShadowComparisonLinearWrap.Get()
        };

        D3D11API::BindSamplerStates(eShader::VertexShader, 0, samplers);
        D3D11API::BindSamplerStates(eShader::PixelShader, 0, samplers);

        // bind viewport
        m_viewport.Bind();
    }

    // camera update
    {
        CB_CAMERA camera;
        camera.cb_eye       = _cameraTransform.translate;
        camera.cb_proj        = _camera.CreateProj();
        camera.cb_view        = _camera.CreateView(_cameraTransform);
        camera.cb_viewProjInv = Mat4(camera.cb_proj * camera.cb_view).Invert();
    }

    // g-buffer pass
    {
        // clear gbuffer m_textures
        m_gBufferNormalTexture.ClearRenderTarget(k_pColorZero);
        m_gBufferAlbedoRoughnessTexture.ClearRenderTarget(k_pColorZero);
        m_gBufferMetallicAOTexture.ClearRenderTarget(k_pColorZero);
        m_gBufferEmissionTexture.ClearRenderTarget(k_pColorZero);
        m_depthTexture.ClearDepthStencil(true, false, 1.f, 0);

        // bind gbuffer m_textures as render targets
        ID3D11RenderTargetView* rtvArray[] = {
            m_gBufferNormalTexture.GetRTV(),
            m_gBufferAlbedoRoughnessTexture.GetRTV(),
            m_gBufferMetallicAOTexture.GetRTV(),
            m_gBufferEmissionTexture.GetRTV()
        };
        D3D11API::BindRenderTargetViews(rtvArray, m_depthTexture.GetDSV());

        // render
    }

    // apply post-processing
    m_postProcess.Apply(m_hdrTexture, _renderTarget);
}

void DemoScene::OnEvent(Event& _event)
{
    m_dispatcher.Dispatch(_event);
}

void DemoScene::OnWindowResizeEvent_(const WindowResizeEvent& _event)
{
    Int32 width  = _event.GetWidth();
    Int32 height = _event.GetHeight();
    CreateScreenDependentResources_(width, height);
}

void DemoScene::OnWindowMaximizeEvent_(const WindowMaximizeEvent& _event)
{
    Int32 width  = _event.GetWidth();
    Int32 height = _event.GetHeight();
    CreateScreenDependentResources_(width, height);
}

void DemoScene::CreateScreenDependentResources_(const Int32 _width, const Int32 _height)
{
    // viewport
    m_viewport = { 0.f, 0.f, static_cast<float>(_width), static_cast<float>(_height) };

    // depth buffer
    m_depthTexture.Initialize(_width, _height, DXGI_FORMAT_R24G8_TYPELESS, eUsage::Default, eViewFlags_DepthStencilView | eViewFlags::ShaderResourceView);
    m_depthTexture.AttachDSV(DXGI_FORMAT_D24_UNORM_S8_UINT);
    m_depthTexture.AttachSRV(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

    // hdr back buffer
    m_hdrTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eUsage::Default, eViewFlags::ShaderResourceView | eViewFlags_RenderTargetView);
    m_hdrTexture.AttachRTV();
    m_hdrTexture.AttachSRV();

    // defered rendering backbuffer
    m_gBufferNormalTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eUsage::Default, eViewFlags::ShaderResourceView | eViewFlags_RenderTargetView);
    m_gBufferNormalTexture.AttachRTV();
    m_gBufferNormalTexture.AttachSRV();

    m_gBufferAlbedoRoughnessTexture.Initialize(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eUsage::Default, eViewFlags::ShaderResourceView | eViewFlags_RenderTargetView);
    m_gBufferAlbedoRoughnessTexture.AttachRTV();
    m_gBufferAlbedoRoughnessTexture.AttachSRV();

    m_gBufferMetallicAOTexture.Initialize(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eUsage::Default, eViewFlags::ShaderResourceView | eViewFlags_RenderTargetView);
    m_gBufferMetallicAOTexture.AttachRTV();
    m_gBufferMetallicAOTexture.AttachSRV();

    m_gBufferEmissionTexture.Initialize(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, eUsage::Default, eViewFlags::ShaderResourceView | eViewFlags_RenderTargetView);
    m_gBufferEmissionTexture.AttachRTV();
    m_gBufferEmissionTexture.AttachSRV();

    // post-process
    PostProcessBuilder builder;
    builder
        .AddBloomFilter(_width, _height, DXGI_FORMAT_R16G16B16A16_FLOAT, 4, m_hdrTexture)
        .AddToneMappingFilter(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eToneMappingFilterType::Linear)
        .AddFXAAFilter(_width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, eFXAAQuality::High);
    m_postProcess = builder.Build();
}