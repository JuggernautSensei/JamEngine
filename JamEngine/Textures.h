#pragma once
#include "RendererCommons.h"

namespace jam
{

enum eViewFlags_ : Int32
{
    eViewFlags_None           = 0,
    eViewFlags_ShaderResource = 1 << 0,
    eViewFlags_RenderTarget   = 1 << 1,
    eViewFlags_DepthStencil   = 1 << 2,
};
using eViewFlags = Int32;

class Texture2D
{
public:
    // initlaizer
    void Initialize(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, eResourceAccess _access = eResourceAccess::Immutable, eViewFlags _viewFlags = eViewFlags_ShaderResource, UInt32 _arraySize = 1, UInt32 _samples = 1, bool _bGenerateMips = false, bool _bCubemap = false, const std::optional<Texture2DInitializeData>& _initializeData = std::nullopt);
    void InitializeFromSwapChain(IDXGISwapChain* _pSwapChain);
    bool LoadFromFile(const fs::path& _filePath, eResourceAccess _access = eResourceAccess::Immutable, eViewFlags _viewFlags = eViewFlags_ShaderResource, bool _bGenrateMips = false, bool _bInverseGamma = false, bool _bCubeMap = false);

    // attachments
    void AttachSRV(DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN);   // if _format is DXGI_FORMAT_UNKNOWN, the format will be the same as the texture format
    void AttachRTV(DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN);   // same
    void AttachDSV(DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN);   // same

    Int32 DetachSRV();
    Int32 DetachRTV();
    Int32 DetachDSV();

    // binding
    void BindAsShaderResource(eShader _shader, UInt32 _slot) const;
    void BindAsRenderTargetWithDepthStencil(const Texture2D& _depthStencil) const;
    void BindAsRenderTarget() const;
    // MRT나 Only-DSV 를 바인드할 때는 수동으로 Renderer::BindRenderTargets() 를 호출해야 함

    // clear
    void ClearRenderTarget(const float _color[4]) const;
    void ClearDepthStencil(bool _bClearDepth, bool _bClearStencil, float _depth = 1.0f, UInt8 _stencil = 0) const;

    NODISCARD bool IsMultiSamplingTexture() const { return m_samples > 1; }
    NODISCARD bool IsCubemap() const { return m_bIsCubemap; }
    NODISCARD bool HasMips() const { return m_bHasMips; }
    NODISCARD bool IsArray() const { return m_arraySize > 1; }

    // d3d11 accessors
    NODISCARD ID3D11Texture2D*          Get() const { return m_texture.Get(); }
    NODISCARD ID3D11Texture2D* const*   GetAddressOf() const { return m_texture.GetAddressOf(); }
    NODISCARD ID3D11ShaderResourceView* GetSRV() const;
    NODISCARD ID3D11RenderTargetView*   GetRTV() const;
    NODISCARD ID3D11DepthStencilView*   GetDSV() const;
    UInt32                              Reset();

private:
    // instance
    ComPtr<ID3D11Texture2D>          m_texture = nullptr;
    ComPtr<ID3D11ShaderResourceView> m_srv     = nullptr;
    ComPtr<ID3D11RenderTargetView>   m_rtv     = nullptr;
    ComPtr<ID3D11DepthStencilView>   m_dsv     = nullptr;

    // texture information
    UInt32          m_width     = 0;
    UInt32          m_height    = 0;
    DXGI_FORMAT     m_format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    UInt32          m_arraySize = 1;
    eResourceAccess m_access    = eResourceAccess::Immutable;
    eViewFlags      m_viewFlags = eViewFlags_None;

    // misc
    UInt32 m_samples    = 1;
    bool   m_bHasMips   = false;
    bool   m_bIsCubemap = false;
};

}   // namespace jam