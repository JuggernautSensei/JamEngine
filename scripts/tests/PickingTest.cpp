#include <DirectXMath.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
constexpr int      kSize   = 64;
constexpr unsigned kNearId = 17;
constexpr unsigned kFarId  = 16777217;

struct Entity
{
    XMFLOAT4X4 world, invTransWorld;
    XMFLOAT3   color = { 0.8f, 0.3f, 0.2f };
    unsigned   id    = 0;
};
struct Selection
{
    unsigned id = 0, showIds = 0;
    int      width   = 1;
    unsigned padding = 0;
};
static_assert(sizeof(Entity) == 144 && sizeof(Selection) == 16);

bool Check(HRESULT _hr, const char* _operation)
{
    if (SUCCEEDED(_hr)) return true;
    std::fprintf(stderr, "FAIL %s: 0x%08lx\n", _operation, static_cast<unsigned long>(_hr));
    return false;
}
bool Expect(bool _condition, const char* _message)
{
    if (!_condition) std::fprintf(stderr, "FAIL %s\n", _message);
    return _condition;
}

class Fixture
{
public:
    bool Init(const std::filesystem::path& _directory, unsigned _flags)
    {
        if (!Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_context), "device")) return false;
        for (int pass = 0; pass < 2; ++pass)
        {
            const auto path = _directory / (pass == 0 ? "PickingShader.hlsl" : "CompositeShader.hlsl");
            for (int stage = 0; stage < 2; ++stage)
            {
                ComPtr<ID3DBlob> code, errors;
                HRESULT          hr = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, stage == 0 ? "VSMain" : "PSMain", stage == 0 ? "vs_5_0" : "ps_5_0", _flags | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
                if (errors) std::fwrite(errors->GetBufferPointer(), 1, errors->GetBufferSize(), stderr);
                if (!Check(hr, "production shader compilation")) return false;
                if (stage == 0)
                {
                    if (!Check(m_device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &m_vs[pass]), "vertex shader")) return false;
                    const D3D11_INPUT_ELEMENT_DESC elements[] = {
                        { "POSITION", 0, pass == 0 ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                        {   "NORMAL", 0,                                        DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
                    };
                    if (!Check(m_device->CreateInputLayout(elements, pass == 0 ? 2 : 1, code->GetBufferPointer(), code->GetBufferSize(), &m_layout[pass]), "input layout")) return false;
                }
                else if (!Check(m_device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &m_ps[pass]), "pixel shader"))
                    return false;
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = desc.Height = kSize;
            desc.MipLevels = desc.ArraySize = 1;
            desc.SampleDesc.Count           = 1;
            desc.Format                     = i == 1 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.BindFlags                  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            if (!Check(m_device->CreateTexture2D(&desc, nullptr, &m_texture[i]), "render texture")
                || !Check(m_device->CreateRenderTargetView(m_texture[i].Get(), nullptr, &m_rtv[i]), "RTV")
                || !Check(m_device->CreateShaderResourceView(m_texture[i].Get(), nullptr, &m_srv[i]), "SRV")) return false;
            desc.Width = desc.Height = 1;
            desc.BindFlags           = 0;
            desc.Usage               = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
            if (!Check(m_device->CreateTexture2D(&desc, nullptr, &m_staging[i]), "staging pixel")) return false;
        }
        D3D11_TEXTURE2D_DESC depth = {};
        depth.Width = depth.Height = kSize;
        depth.MipLevels = depth.ArraySize = 1;
        depth.SampleDesc.Count            = 1;
        depth.Format                      = DXGI_FORMAT_D32_FLOAT;
        depth.BindFlags                   = D3D11_BIND_DEPTH_STENCIL;
        ComPtr<ID3D11Texture2D> depthTexture;
        if (!Check(m_device->CreateTexture2D(&depth, nullptr, &depthTexture), "depth")
            || !Check(m_device->CreateDepthStencilView(depthTexture.Get(), nullptr, &m_depth), "DSV")) return false;
        D3D11_DEPTH_STENCIL_DESC ds = {};
        ds.DepthEnable              = true;
        ds.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
        ds.DepthFunc                = D3D11_COMPARISON_LESS;
        if (!Check(m_device->CreateDepthStencilState(&ds, &m_depthState), "depth state")) return false;
        ds.DepthEnable = false;
        if (!Check(m_device->CreateDepthStencilState(&ds, &m_noDepthState), "no depth state")) return false;
        D3D11_RASTERIZER_DESC raster = {};
        raster.FillMode              = D3D11_FILL_SOLID;
        raster.CullMode              = D3D11_CULL_NONE;
        raster.DepthClipEnable       = true;
        ComPtr<ID3D11RasterizerState> rasterState;
        if (!Check(m_device->CreateRasterizerState(&raster, &rasterState), "raster state")) return false;
        m_context->RSSetState(rasterState.Get());
        D3D11_VIEWPORT viewport = { 0.f, 0.f, kSize, kSize, 0.f, 1.f };
        m_context->RSSetViewports(1, &viewport);
        D3D11_BUFFER_DESC cb = {};
        cb.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        cb.ByteWidth         = sizeof(Entity);
        if (!Check(m_device->CreateBuffer(&cb, nullptr, &m_entity), "entity constants")) return false;
        cb.ByteWidth = sizeof(XMFLOAT4X4);
        XMFLOAT4X4 identity;
        XMStoreFloat4x4(&identity, XMMatrixIdentity());
        D3D11_SUBRESOURCE_DATA identityData = { &identity, 0, 0 };
        if (!Check(m_device->CreateBuffer(&cb, &identityData, &m_camera), "camera constants")) return false;
        cb.ByteWidth = sizeof(Selection);
        if (!Check(m_device->CreateBuffer(&cb, nullptr, &m_selection), "selection constants")) return false;
        const float       vertices[] = { -0.9f, -0.9f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.9f, 0.f, 0.f, 0.f, -1.f, 0.9f, -0.9f, 0.f, 0.f, 0.f, -1.f };
        D3D11_BUFFER_DESC vb         = {};
        vb.BindFlags                 = D3D11_BIND_VERTEX_BUFFER;
        vb.ByteWidth                 = sizeof(vertices);
        D3D11_SUBRESOURCE_DATA data  = { vertices, 0, 0 };
        if (!Check(m_device->CreateBuffer(&vb, &data, &m_vertices[0]), "triangle")) return false;
        const float screen[] = { -1.f, 1.f, 3.f, 1.f, -1.f, -3.f };
        vb.ByteWidth         = sizeof(screen);
        data.pSysMem         = screen;
        return Check(m_device->CreateBuffer(&vb, &data, &m_vertices[1]), "screen triangle");
    }

    void Clear()
    {
        ID3D11ShaderResourceView* nulls[2] = {};
        m_context->PSSetShaderResources(0, 2, nulls);
        const float zero[4] = {};
        m_context->ClearRenderTargetView(m_rtv[0].Get(), zero);
        m_context->ClearRenderTargetView(m_rtv[1].Get(), zero);
        m_context->ClearDepthStencilView(m_depth.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
    }

    void Draw(unsigned _id, float _scale, float _depth, float _x = 0.f, float _y = 0.f)
    {
        Bind(0);
        ID3D11RenderTargetView* targets[] = { m_rtv[0].Get(), m_rtv[1].Get() };
        m_context->OMSetRenderTargets(2, targets, m_depth.Get());
        m_context->OMSetDepthStencilState(m_depthState.Get(), 0);
        Entity entity;
        XMStoreFloat4x4(&entity.world, XMMatrixTranspose(XMMatrixScaling(_scale, _scale, 1.f) * XMMatrixTranslation(_x, _y, _depth)));
        XMStoreFloat4x4(&entity.invTransWorld, XMMatrixIdentity());
        entity.id = _id;
        m_context->UpdateSubresource(m_entity.Get(), 0, nullptr, &entity, 0, 0);
        m_context->VSSetConstantBuffers(0, 1, m_entity.GetAddressOf());
        m_context->VSSetConstantBuffers(1, 1, m_camera.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, m_entity.GetAddressOf());
        m_context->Draw(3, 0);
    }

    void Composite(unsigned _id, bool _showIds = false, int _width = 1)
    {
        m_context->OMSetRenderTargets(1, m_rtv[2].GetAddressOf(), nullptr);
        m_context->OMSetDepthStencilState(m_noDepthState.Get(), 0);
        Bind(1);
        const Selection selection = { _id, _showIds ? 1u : 0u, _width, 0 };
        m_context->UpdateSubresource(m_selection.Get(), 0, nullptr, &selection, 0, 0);
        m_context->PSSetConstantBuffers(0, 1, m_selection.GetAddressOf());
        ID3D11ShaderResourceView* resources[] = { m_srv[0].Get(), m_srv[1].Get() };
        m_context->PSSetShaderResources(0, 2, resources);
        m_context->Draw(3, 0);
    }

    unsigned Pixel(int _texture, int _x, int _y)
    {
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        const D3D11_BOX box = { static_cast<UINT>(_x), static_cast<UINT>(_y), 0, static_cast<UINT>(_x + 1), static_cast<UINT>(_y + 1), 1 };
        m_context->CopySubresourceRegion(m_staging[_texture].Get(), 0, 0, 0, 0, m_texture[_texture].Get(), 0, &box);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (!Check(m_context->Map(m_staging[_texture].Get(), 0, D3D11_MAP_READ, 0, &mapped), "read pixel")) return ~0u;
        unsigned value;
        std::memcpy(&value, mapped.pData, sizeof(value));
        m_context->Unmap(m_staging[_texture].Get(), 0);
        return value;
    }

private:
    void Bind(int _pass)
    {
        const UINT stride = _pass == 0 ? 24 : 8, offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_vertices[_pass].GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->IASetInputLayout(m_layout[_pass].Get());
        m_context->VSSetShader(m_vs[_pass].Get(), nullptr, 0);
        m_context->PSSetShader(m_ps[_pass].Get(), nullptr, 0);
    }

    ComPtr<ID3D11Device>                            m_device;
    ComPtr<ID3D11DeviceContext>                     m_context;
    std::array<ComPtr<ID3D11VertexShader>, 2>       m_vs;
    std::array<ComPtr<ID3D11PixelShader>, 2>        m_ps;
    std::array<ComPtr<ID3D11InputLayout>, 2>        m_layout;
    std::array<ComPtr<ID3D11Buffer>, 2>             m_vertices;
    std::array<ComPtr<ID3D11Texture2D>, 3>          m_texture, m_staging;
    std::array<ComPtr<ID3D11RenderTargetView>, 3>   m_rtv;
    std::array<ComPtr<ID3D11ShaderResourceView>, 3> m_srv;
    ComPtr<ID3D11DepthStencilView>                  m_depth;
    ComPtr<ID3D11DepthStencilState>                 m_depthState, m_noDepthState;
    ComPtr<ID3D11Buffer>                            m_entity, m_camera, m_selection;
};

bool Test(Fixture& _gpu)
{
    for (bool reverse: { false, true })
    {
        _gpu.Clear();
        if (reverse) _gpu.Draw(kNearId, 0.45f, 0.2f);
        _gpu.Draw(kFarId, 1.f, 0.6f);
        if (!reverse) _gpu.Draw(kNearId, 0.45f, 0.2f);
        if (!Expect(_gpu.Pixel(1, 0, 0) == 0, "clear/background ID is zero")
            || !Expect(_gpu.Pixel(1, 32, 32) == kNearId, "nearest visible ID, both submission orders")
            || !Expect(_gpu.Pixel(1, 32, 55) == kFarId, "large integer ID survives MRT and 1x1 copy")) return false;
    }
    if (!Expect(_gpu.Pixel(1, 25, 32) == kNearId && _gpu.Pixel(1, 24, 32) != kNearId, "test silhouette boundary")) return false;
    const unsigned original = _gpu.Pixel(0, 25, 32);
    _gpu.Composite(0);
    if (!Expect(_gpu.Pixel(2, 25, 32) == original, "ID zero disables selection overlay")) return false;
    _gpu.Composite(kNearId);
    unsigned outlined = _gpu.Pixel(2, 25, 32);
    if (!Expect((outlined & 255) > 240 && ((outlined >> 8) & 255) > 180 && ((outlined >> 16) & 255) < 40, "visible silhouette is outlined")) return false;
    _gpu.Composite(0, true);
    if (!Expect(_gpu.Pixel(2, 0, 0) == 0xff000000u && _gpu.Pixel(2, 32, 55) != original, "ID visualization")) return false;
    _gpu.Clear();
    _gpu.Draw(kNearId, 0.45f, 0.2f);
    _gpu.Draw(0, 0.01f, 0.1f, 1.f / 64.f, -1.f / 64.f);
    if (!Expect(_gpu.Pixel(1, 32, 32) == 0 && _gpu.Pixel(1, 31, 32) == kNearId, "one-pixel hole fixture")) return false;
    _gpu.Composite(kNearId, false, 2);
    outlined = _gpu.Pixel(2, 31, 32);
    if (!Expect((outlined & 255) > 240 && ((outlined >> 8) & 255) > 180 && ((outlined >> 16) & 255) < 40, "thick outline includes narrow holes")) return false;
    _gpu.Clear();
    if (!Expect(_gpu.Pixel(1, 32, 32) == 0, "clear removes previous frame IDs")) return false;
    std::puts("PASS clear 0, overlap/depth, exact IDs, 1x1 readback, outline, deselection, ID visualization");
    return true;
}
}   // namespace

int wmain(int _argc, wchar_t** _argv)
{
    if (_argc != 2) return 2;
    for (unsigned flags: { D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, D3DCOMPILE_OPTIMIZATION_LEVEL3 })
    {
        Fixture gpu;
        if (!gpu.Init(_argv[1], flags) || !Test(gpu)) return 1;
    }
    return 0;
}
