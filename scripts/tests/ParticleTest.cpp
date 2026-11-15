#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
constexpr int kCapacity = 100000;

struct Particle
{
    float    position[3], age;
    float    velocity[3], lifetime;
    float    color[4];
    float    angle, angularVelocity;
    unsigned seed;
    float    size;
};

struct Params
{
    float deltaTime = 0.f, time = 0.f, wind = 1.f, size = 1.f;
    int   count = 0, previousCount = 0, effect = 0, seed = 1;
    float extent[3] = { 12.f, 10.f, 8.f };
    float fallSpeed = 1.f;
};
static_assert(sizeof(Particle) == 64 && sizeof(Params) == 48);

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
        ComPtr<ID3DBlob> compute;
        for (const auto& stage: {
                 std::pair {   L"SimulateShader.hlsl", "cs_5_0" },
                 {   L"ParticleShader.hlsl", "vs_5_0" },
                 {   L"ParticleShader.hlsl", "ps_5_0" },
                 { L"BackgroundShader.hlsl", "vs_5_0" },
                 { L"BackgroundShader.hlsl", "ps_5_0" }
        })
        {
            ComPtr<ID3DBlob> code, errors;
            const char*      entry = stage.second[0] == 'c' ? "CSMain" : stage.second[0] == 'v' ? "VSMain"
                                                                                                : "PSMain";
            const auto       path  = _directory / stage.first;
            const HRESULT    hr    = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, stage.second, _flags | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
            if (errors) std::fwrite(errors->GetBufferPointer(), 1, errors->GetBufferSize(), stderr);
            if (!Check(hr, "production shader compilation")) return false;
            if (stage.second[0] == 'c') compute = code;
        }
        if (!Check(m_device->CreateComputeShader(compute->GetBufferPointer(), compute->GetBufferSize(), nullptr, &m_shader), "compute shader")) return false;
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth         = sizeof(Params);
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        if (!Check(m_device->CreateBuffer(&desc, nullptr, &m_params), "constants")) return false;
        desc.ByteWidth           = kCapacity * sizeof(Particle);
        desc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(Particle);
        if (!Check(m_device->CreateBuffer(&desc, nullptr, &m_particles), "particles")) return false;
        if (!Check(m_device->CreateUnorderedAccessView(m_particles.Get(), nullptr, &m_uav), "UAV")) return false;
        desc.BindFlags           = 0;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;
        desc.Usage               = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        return Check(m_device->CreateBuffer(&desc, nullptr, &m_readback), "readback");
    }

    void Fill(const std::vector<Particle>& _particles)
    {
        m_context->UpdateSubresource(m_particles.Get(), 0, nullptr, _particles.data(), 0, 0);
    }

    std::vector<Particle> Run(const Params& _params)
    {
        m_context->UpdateSubresource(m_params.Get(), 0, nullptr, &_params, 0, 0);
        m_context->CSSetShader(m_shader.Get(), nullptr, 0);
        m_context->CSSetConstantBuffers(0, 1, m_params.GetAddressOf());
        m_context->CSSetUnorderedAccessViews(0, 1, m_uav.GetAddressOf(), nullptr);
        // 0개 디스패치도 한 번 실행 (셰이더 가드 확인)
        m_context->Dispatch((_params.count + 255) / 256 + (_params.count == 0), 1, 1);
        ID3D11UnorderedAccessView* nullUav = nullptr;
        m_context->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
        m_context->CopyResource(m_readback.Get(), m_particles.Get());
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (!Check(m_context->Map(m_readback.Get(), 0, D3D11_MAP_READ, 0, &mapped), "map")) return {};
        std::vector<Particle> result(kCapacity);
        std::memcpy(result.data(), mapped.pData, result.size() * sizeof(Particle));
        m_context->Unmap(m_readback.Get(), 0);
        return result;
    }

private:
    ComPtr<ID3D11Device>              m_device;
    ComPtr<ID3D11DeviceContext>       m_context;
    ComPtr<ID3D11ComputeShader>       m_shader;
    ComPtr<ID3D11Buffer>              m_params, m_particles, m_readback;
    ComPtr<ID3D11UnorderedAccessView> m_uav;
};

bool Valid(const Particle& _p)
{
    return std::isfinite(_p.position[0]) && std::isfinite(_p.position[1]) && std::isfinite(_p.position[2])
        && std::isfinite(_p.velocity[0]) && std::isfinite(_p.velocity[1]) && std::isfinite(_p.velocity[2])
        && std::isfinite(_p.angle) && std::isfinite(_p.angularVelocity) && std::isfinite(_p.size)
        && _p.position[1] > 0.f && _p.position[1] <= 10.f && _p.lifetime > 0.f
        && _p.age >= 0.f && _p.age < _p.lifetime && _p.size > 0.f && _p.seed != 0;
}

bool Test(Fixture& _gpu, int _effect)
{
    Particle sentinel;
    std::memset(&sentinel, 0xff, sizeof(sentinel));
    const std::vector<Particle> untouched(kCapacity, sentinel);
    Params                      params;
    params.effect = _effect;
    for (int count: { 0, 1, 255, 256, 257, kCapacity })
    {
        _gpu.Fill(untouched);
        params.count = count;
        auto result  = _gpu.Run(params);
        if (!Expect(result.size() == kCapacity, "readback size")) return false;
        for (int i = 0; i < count; ++i)
            if (!Expect(Valid(result[i]), "new slots initialized while paused")) return false;
        if (!Expect(std::memcmp(result.data() + count, untouched.data() + count, (kCapacity - count) * sizeof(Particle)) == 0, "inactive tail unchanged")) return false;
    }

    params.count         = 257;
    params.previousCount = 0;
    auto original        = _gpu.Run(params);
    params.previousCount = 257;
    params.count         = 1;
    auto shrunk          = _gpu.Run(params);
    if (!Expect(std::memcmp(original.data(), shrunk.data(), kCapacity * sizeof(Particle)) == 0, "pause preserves existing state")) return false;
    params.previousCount = 1;
    params.count         = 257;
    params.seed          = 17;
    auto grown           = _gpu.Run(params);
    if (!Expect(std::memcmp(&grown[0], &original[0], sizeof(Particle)) == 0, "grow preserves active prefix")) return false;
    for (int i = 1; i < 257; ++i)
        if (!Expect(Valid(grown[i]) && grown[i].seed != original[i].seed, "reactivated slots reset")) return false;
    if (!Expect(std::memcmp(grown.data() + 257, original.data() + 257, (kCapacity - 257) * sizeof(Particle)) == 0, "grow preserves inactive tail")) return false;

    params.previousCount = 257;
    params.deltaTime     = 1.f / 60.f;
    auto moved           = _gpu.Run(params);
    if (!Expect(moved[0].age > grown[0].age && moved[0].position[1] < grown[0].position[1], "simulation advances")) return false;
    moved[0].age         = moved[0].lifetime + 1.f;
    moved[1].position[1] = -10.f;
    _gpu.Fill(moved);
    auto respawned = _gpu.Run(params);
    if (!Expect(Valid(respawned[0]) && Valid(respawned[1]) && respawned[0].age < 0.1f, "expired and ground slots respawn")) return false;
    params.previousCount = 0;
    params.deltaTime     = 0.f;
    auto reset           = _gpu.Run(params);
    auto repeated        = _gpu.Run(params);
    if (!Expect(Valid(reset[0]) && std::memcmp(reset.data(), repeated.data(), 257 * sizeof(Particle)) == 0, "restart deterministic")) return false;
    params.count         = 0;
    params.previousCount = 257;
    auto zero            = _gpu.Run(params);
    if (!Expect(std::memcmp(zero.data(), reset.data(), kCapacity * sizeof(Particle)) == 0, "zero count preserves storage")) return false;
    params.count         = 257;
    params.previousCount = 0;
    auto reenabled       = _gpu.Run(params);
    if (!Expect(std::memcmp(reenabled.data(), reset.data(), 257 * sizeof(Particle)) == 0, "zero to active initializes")) return false;
    std::printf("PASS %s: counts, inactive tail, shrink/grow, pause, motion, respawn, restart\n", _effect == 0 ? "snow" : "confetti");
    return true;
}
}   // namespace

int wmain(int _argc, wchar_t** _argv)
{
    if (_argc != 2) return 2;
    for (unsigned flags: { D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, D3DCOMPILE_OPTIMIZATION_LEVEL3 })
    {
        Fixture gpu;
        if (!gpu.Init(_argv[1], flags) || !Test(gpu, 0) || !Test(gpu, 1)) return 1;
    }
    return 0;
}
