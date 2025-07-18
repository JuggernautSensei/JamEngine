#pragma once
#include "Renderer.h"
#include "RendererCommons.h"

namespace jam
{

struct InitializeTextureData
{
    const void* pInitialData = nullptr;
    UInt32      pitch        = 0;
};

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
    static NODISCARD Texture2D Create(UInt32 _width, UInt32 _height, DXGI_FORMAT _format, UInt32 _arraySize, UInt32 _samples, bool _generateMips, bool _cubemap, eResourceAccess _access, eViewFlags _viewFlags, const InitializeTextureData* _pInitialData_orNull = nullptr);
    static NODISCARD std::optional<Texture2D> CreateFromFile(const fs::path& _filePath, eViewFlags _viewFlags, bool _bGenrateMips, bool _bInverseGamma);

    void AttachSRV(DXGI_FORMAT _format);
    void AttachRTV(DXGI_FORMAT _format);
    void AttachDSV(DXGI_FORMAT _format);

    void BindAsShaderResource(eShader _shader, UInt32 _slot) const;
    void BindAsOutputResource(bool _bBindRenderTarget, bool _bBindDepthStencil) const;

    NODISCARD bool IsMultiSamplingTexture() const { return m_samples > 1; }
    NODISCARD bool IsCubemap() const { return m_bIsCubemap; }
    NODISCARD bool HasMips() const { return m_bHasMips; }
    NODISCARD bool IsArray() const { return m_arraySize > 1; }

    // d3d11 accessors
    NODISCARD const ID3D11Texture2D*        Get() const { return m_texture.Get(); }
    NODISCARD const ID3D11Texture2D* const* GetAddressOf() const { return m_texture.GetAddressOf(); }
    NODISCARD ID3D11ShaderResourceView*     GetSRV() const;
    NODISCARD ID3D11RenderTargetView*       GetRTV() const;
    NODISCARD ID3D11DepthStencilView*       GetDSV() const;

private:
    // instance
    ComPtr<ID3D11Texture2D>          m_texture = nullptr;
    ComPtr<ID3D11ShaderResourceView> m_srv     = nullptr;
    ComPtr<ID3D11RenderTargetView>   m_rtv     = nullptr;
    ComPtr<ID3D11DepthStencilView>   m_dsv     = nullptr;

    // texture information
    UInt32      m_width     = 0;
    UInt32      m_height    = 0;
    DXGI_FORMAT m_format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    UInt32      m_arraySize = 1;

    // misc
    UInt32 m_samples    = 1;
    bool   m_bHasMips   = false;
    bool   m_bIsCubemap = false;

    // access flags
    bool m_bGPUWrite = false;
    bool m_bCPURead  = false;   // staging texture
    bool m_bCPUWrite = false;

    // bind flags
    eViewFlags m_bindFlags = eViewFlags_None;
};

}   // namespace jam