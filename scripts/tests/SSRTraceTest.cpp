#include <DirectXMath.h>
#include <cstdio>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
constexpr int kSize    = 64;
constexpr int kOriginX = 32;
constexpr int kOriginY = 48;

struct Color
{
    float r, g, b, a;
};

struct Params
{
    XMFLOAT4X4 proj, invProj;
    XMFLOAT2   screenSize   = { kSize, kSize };
    float      maxDistance  = 30.f;
    float      thickness    = 0.1f;
    float      steps        = 96.f;
    float      edgeFade     = 0.1f;
    float      bSkyFallback = 0.f;
    float      padding1     = 0.f;
    XMFLOAT3   skyColor     = {};
    float      padding2     = 0.f;
};
static_assert(sizeof(Params) == 176);

bool Check(const HRESULT _result, const char* _operation)
{
    if (SUCCEEDED(_result))
        return true;
    std::fprintf(stderr, "%s failed: 0x%08lx\n", _operation, static_cast<unsigned long>(_result));
    return false;
}

std::string Read(const std::filesystem::path& _path)
{
    std::ifstream stream(_path, std::ios::binary);
    return { std::istreambuf_iterator<char>(stream),
             std::istreambuf_iterator<char>() };
}

class Fixture
{
public:
    bool Init(const std::filesystem::path& _shaderPath)
    {
        if (!Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_context),
                   "CreateDevice"))
            return false;
        std::string       source = Read(_shaderPath);
        const std::string fullscreen =
            Read(_shaderPath.parent_path() / "Fullscreen.hlsl");
        const std::string include = "#include \"Fullscreen.hlsl\"";
        const auto        offset  = source.find(include);
        if (source.empty() || fullscreen.empty() || offset == std::string::npos)
            return false;
        source.replace(offset, include.size(), fullscreen);
        for (const unsigned flags: { D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS, D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS })
        {
            ComPtr<ID3DBlob> pixelShader, pixelErrors;
            const HRESULT    compiled = D3DCompile(source.data(), source.size(), "SSRTraceTest", nullptr, nullptr, "PSMain", "ps_5_0", flags, 0, &pixelShader, &pixelErrors);
            if (pixelErrors) std::fwrite(pixelErrors->GetBufferPointer(), 1, pixelErrors->GetBufferSize(), stderr);
            if (!Check(compiled, "CompilePixelShader")) return false;
        }
        source += R"(
RWStructuredBuffer<float4> testOutput : register(u0);
[numthreads(1, 1, 1)]
void CSMain()
{
    PSInput input;
    input.posH = float4(32.5f, 48.5f, 0.0f, 1.0f);
    testOutput[0] = PSMain(input);
}
)";
        ComPtr<ID3DBlob> shader, errors;
        const HRESULT    compiled = D3DCompile(
            source.data(), source.size(), "SSRTraceTest", nullptr, nullptr, "CSMain", "cs_5_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &shader, &errors);
        if (errors)
            std::fwrite(errors->GetBufferPointer(), 1, errors->GetBufferSize(), stderr);
        if (!Check(compiled, "CompileShader"))
            return false;
        if (!Check(m_device->CreateComputeShader(shader->GetBufferPointer(),
                                                 shader->GetBufferSize(),
                                                 nullptr,
                                                 &m_shader),
                   "CreateShader"))
            return false;

        Params         params;
        const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 100.f);
        XMStoreFloat4x4(&params.proj, XMMatrixTranspose(proj));
        XMStoreFloat4x4(&params.invProj,
                        XMMatrixTranspose(XMMatrixInverse(nullptr, proj)));
        D3D11_BUFFER_DESC cb                = {};
        cb.ByteWidth                        = sizeof(params);
        cb.BindFlags                        = D3D11_BIND_CONSTANT_BUFFER;
        const D3D11_SUBRESOURCE_DATA cbData = { &params, 0, 0 };
        if (!Check(m_device->CreateBuffer(&cb, &cbData, &m_params), "CreateParams"))
            return false;

        D3D11_BUFFER_DESC output   = {};
        output.ByteWidth           = sizeof(Color);
        output.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
        output.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        output.StructureByteStride = sizeof(Color);
        if (!Check(m_device->CreateBuffer(&output, nullptr, &m_output),
                   "CreateOutput"))
            return false;
        if (!Check(m_device->CreateUnorderedAccessView(m_output.Get(), nullptr, &m_uav),
                   "CreateUav"))
            return false;
        output.BindFlags = output.MiscFlags = output.StructureByteStride = 0;
        output.Usage                                                     = D3D11_USAGE_STAGING;
        output.CPUAccessFlags                                            = D3D11_CPU_ACCESS_READ;
        return Check(m_device->CreateBuffer(&output, nullptr, &m_readback),
                     "CreateReadback");
    }

    bool Run(const char* _name, const bool _bForeground, const bool _bMiss, const bool _bTiltedNormal = false, const bool _bBoundary = false)
    {
        std::vector<float> depths(kSize * kSize, 1.f);
        std::vector<Color> normals(kSize * kSize, { 0.f, 1.f, 0.f, 1.f });
        std::vector<Color> colors(kSize * kSize, { 0.2f, 0.2f, 0.2f, 1.f });
        const float        floorY  = (1.f - 2.f * (kOriginY + 0.5f) / kSize) * 5.f;
        const auto         depthOf = [](const float _z) {
            return 100.f / 99.9f - (10.f / 99.9f) / _z;
        };
        for (int y = 0; y < kSize; ++y)
        {
            const float ndcY = 1.f - 2.f * (y + 0.5f) / kSize;
            const float z    = floorY / ndcY;
            for (int x = 0; x < kSize; ++x)
            {
                depths[y * kSize + x] = z >= 0.1f && z <= 100.f ? depthOf(z) : 1.f;
            }
        }
        const int targetY = _bForeground ? 46 : 47;
        for (int x = 0; x < kSize; ++x)
        {
            const int target = targetY * kSize + x;
            depths[target]   = depthOf(_bMiss ? 2.f : (_bBoundary ? 5.03f : (_bForeground ? 5.3f : 5.12f)));
            normals[target]  = _bTiltedNormal ? Color { 0.f, 1.f, 0.f, 0.f } : Color { 0.f, 0.f, -1.f, 0.f };
            colors[target]   = { 1.f, 0.f, 0.f, 1.f };
            if (_bForeground)
            {
                const int foreground = 47 * kSize + x;
                depths[foreground]   = depthOf(2.f);
                normals[foreground]  = { 0.f, 0.f, -1.f, 0.f };
                colors[foreground]   = { 0.f, 1.f, 0.f, 1.f };
            }
        }
        ComPtr<ID3D11ShaderResourceView> views[3];
        if (!Texture(normals.data(), sizeof(Color), DXGI_FORMAT_R32G32B32A32_FLOAT, views[0]))
            return false;
        if (!Texture(depths.data(), sizeof(float), DXGI_FORMAT_R32_FLOAT, views[1]))
            return false;
        if (!Texture(colors.data(), sizeof(Color), DXGI_FORMAT_R32G32B32A32_FLOAT, views[2]))
            return false;
        ID3D11ShaderResourceView* resources[] = { views[0].Get(), views[1].Get(), views[2].Get() };
        m_context->CSSetShader(m_shader.Get(), nullptr, 0);
        m_context->CSSetShaderResources(0, 3, resources);
        m_context->CSSetConstantBuffers(0, 1, m_params.GetAddressOf());
        m_context->CSSetUnorderedAccessViews(0, 1, m_uav.GetAddressOf(), nullptr);
        m_context->Dispatch(1, 1, 1);
        m_context->CopyResource(m_readback.Get(), m_output.Get());
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (!Check(m_context->Map(m_readback.Get(), 0, D3D11_MAP_READ, 0, &mapped),
                   "Readback"))
            return false;
        const Color result = *static_cast<const Color*>(mapped.pData);
        m_context->Unmap(m_readback.Get(), 0);
        const bool passed =
            _bMiss ? result.a < 0.01f
                   : result.r > 0.9f && result.g < 0.01f && result.a > 0.9f;
        std::printf("%s %s: rgba=(%.3f, %.3f, %.3f, %.3f)\n",
                    passed ? "PASS" : "FAIL",
                    _name,
                    result.r,
                    result.g,
                    result.b,
                    result.a);
        return passed;
    }

private:
    bool Texture(const void* _data, const unsigned _stride, const DXGI_FORMAT _format, ComPtr<ID3D11ShaderResourceView>& _view)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = desc.Height = kSize;
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format                                             = _format;
        desc.BindFlags                                          = D3D11_BIND_SHADER_RESOURCE;
        const D3D11_SUBRESOURCE_DATA data                       = { _data, kSize * _stride, 0 };
        ComPtr<ID3D11Texture2D>      texture;
        return Check(m_device->CreateTexture2D(&desc, &data, &texture),
                     "CreateTexture")
            && Check(m_device->CreateShaderResourceView(texture.Get(), nullptr, &_view),
                     "CreateSrv");
    }

    ComPtr<ID3D11Device>              m_device;
    ComPtr<ID3D11DeviceContext>       m_context;
    ComPtr<ID3D11ComputeShader>       m_shader;
    ComPtr<ID3D11Buffer>              m_params, m_output, m_readback;
    ComPtr<ID3D11UnorderedAccessView> m_uav;
};
}   // namespace

int wmain(const int _argc, wchar_t** _argv)
{
    if (_argc != 2)
        return 2;
    Fixture fixture;
    if (!fixture.Init(_argv[1]))
        return 2;
    int failures = 0;
    failures += !fixture.Run("ThinVisibleReflector", false, false);
    failures += !fixture.Run("VisibleReflectorBeyondForegroundEdge", true, false);
    failures +=
        !fixture.Run("ForegroundDepthIsNotAnInfiniteSurface", false, true);
    failures += !fixture.Run("SingleLayerDepthHitWithTiltedNormal", false, false, true);
    failures += !fixture.Run("PixelEntryUsesValidatedSurfaceColor", false, false, false, true);
    return failures ? 1 : 0;
}
