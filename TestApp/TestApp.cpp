#include <chrono>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// 상수 버퍼 구조체
struct ShaderConstants
{
    int   branchMode;   // 0: 정적 균일, 1: 동적 균일, 2: 동적 발산
    float threshold;    // 분기 임계값
    float padding[2];
};

// 정적 분기 - 경로 A만 (균일)
const char* g_StaticShaderA = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 복잡한 수학 연산 A
    for (int i = 0; i < 100; i++) {
        float x = input.uv.x * 4.0 - 2.0;
        float y = input.uv.y * 4.0 - 2.0;
        
        float zx = 0.0;
        float zy = 0.0;
        
        for (int j = 0; j < 50; j++) {
            float xtemp = zx * zx - zy * zy + x;
            zy = 2.0 * zx * zy + y;
            zx = xtemp;
            
            if (zx * zx + zy * zy > 4.0) break;
            
            color += float3(
                sin(zx * 3.14159) * cos(zy * 2.71828),
                cos(zx * 2.71828) * sin(zy * 3.14159),
                sin(zx * zy * 1.41421)
            );
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 정적 분기 - 경로 B만 (균일)
const char* g_StaticShaderB = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 복잡한 수학 연산 B
    for (int i = 0; i < 100; i++) {
        float u = input.uv.x * 6.28318;
        float v = input.uv.y * 6.28318;
        
        for (int j = 0; j < 50; j++) {
            float t = float(j) * 0.1;
            
            color += float3(
                sin(u * cos(v + t)) * cos(u * sin(v - t)),
                cos(v * sin(u + t)) * sin(v * cos(u - t)),
                sin(u * v * t) * cos(u + v + t)
            );
            
            u = fmod(u + color.r * 0.1, 6.28318);
            v = fmod(v + color.g * 0.1, 6.28318);
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 동적 분기 - 상수 버퍼로 균일 분기 (warp 내 모든 스레드가 동일 경로)
const char* g_DynamicUniformShader = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 상수 버퍼로 균일 분기 - 모든 픽셀이 같은 경로
    if (branchMode == 0) {
        // 경로 A
        for (int i = 0; i < 100; i++) {
            float x = input.uv.x * 4.0 - 2.0;
            float y = input.uv.y * 4.0 - 2.0;
            
            float zx = 0.0;
            float zy = 0.0;
            
            for (int j = 0; j < 50; j++) {
                float xtemp = zx * zx - zy * zy + x;
                zy = 2.0 * zx * zy + y;
                zx = xtemp;
                
                if (zx * zx + zy * zy > 4.0) break;
                
                color += float3(
                    sin(zx * 3.14159) * cos(zy * 2.71828),
                    cos(zx * 2.71828) * sin(zy * 3.14159),
                    sin(zx * zy * 1.41421)
                );
            }
        }
    } else {
        // 경로 B
        for (int i = 0; i < 100; i++) {
            float u = input.uv.x * 6.28318;
            float v = input.uv.y * 6.28318;
            
            for (int j = 0; j < 50; j++) {
                float t = float(j) * 0.1;
                
                color += float3(
                    sin(u * cos(v + t)) * cos(u * sin(v - t)),
                    cos(v * sin(u + t)) * sin(v * cos(u - t)),
                    sin(u * v * t) * cos(u + v + t)
                );
                
                u = fmod(u + color.r * 0.1, 6.28318);
                v = fmod(v + color.g * 0.1, 6.28318);
            }
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// SWITCH 문을 사용한 균일 분기
const char* g_SwitchUniformShader = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // SWITCH 문으로 균일 분기 - 모든 픽셀이 같은 경로
    switch (branchMode) {
        case 0:
            // 경로 A
            for (int i = 0; i < 100; i++) {
                float x = input.uv.x * 4.0 - 2.0;
                float y = input.uv.y * 4.0 - 2.0;
                
                float zx = 0.0;
                float zy = 0.0;
                
                for (int j = 0; j < 50; j++) {
                    float xtemp = zx * zx - zy * zy + x;
                    zy = 2.0 * zx * zy + y;
                    zx = xtemp;
                    
                    if (zx * zx + zy * zy > 4.0) break;
                    
                    color += float3(
                        sin(zx * 3.14159) * cos(zy * 2.71828),
                        cos(zx * 2.71828) * sin(zy * 3.14159),
                        sin(zx * zy * 1.41421)
                    );
                }
            }
            break;
            
        case 1:
            // 경로 B
            for (int i = 0; i < 100; i++) {
                float u = input.uv.x * 6.28318;
                float v = input.uv.y * 6.28318;
                
                for (int j = 0; j < 50; j++) {
                    float t = float(j) * 0.1;
                    
                    color += float3(
                        sin(u * cos(v + t)) * cos(u * sin(v - t)),
                        cos(v * sin(u + t)) * sin(v * cos(u - t)),
                        sin(u * v * t) * cos(u + v + t)
                    );
                    
                    u = fmod(u + color.r * 0.1, 6.28318);
                    v = fmod(v + color.g * 0.1, 6.28318);
                }
            }
            break;
            
        default:
            color = float3(1, 0, 0); // 에러 표시
            break;
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 동적 분기 - 픽셀마다 다른 경로 (warp divergence 발생!)
const char* g_DynamicDivergentShader = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 픽셀마다 다른 경로 선택 - WARP DIVERGENCE!
    // UV 좌표에 따라 다른 경로를 실행하므로, 같은 warp 내 스레드들이 서로 다른 경로를 실행
    if (input.uv.x < threshold) {
        // 경로 A - 화면 왼쪽 절반
        for (int i = 0; i < 100; i++) {
            float x = input.uv.x * 4.0 - 2.0;
            float y = input.uv.y * 4.0 - 2.0;
            
            float zx = 0.0;
            float zy = 0.0;
            
            for (int j = 0; j < 50; j++) {
                float xtemp = zx * zx - zy * zy + x;
                zy = 2.0 * zx * zy + y;
                zx = xtemp;
                
                if (zx * zx + zy * zy > 4.0) break;
                
                color += float3(
                    sin(zx * 3.14159) * cos(zy * 2.71828),
                    cos(zx * 2.71828) * sin(zy * 3.14159),
                    sin(zx * zy * 1.41421)
                );
            }
        }
    } else {
        // 경로 B - 화면 오른쪽 절반
        for (int i = 0; i < 100; i++) {
            float u = input.uv.x * 6.28318;
            float v = input.uv.y * 6.28318;
            
            for (int j = 0; j < 50; j++) {
                float t = float(j) * 0.1;
                
                color += float3(
                    sin(u * cos(v + t)) * cos(u * sin(v - t)),
                    cos(v * sin(u + t)) * sin(v * cos(u - t)),
                    sin(u * v * t) * cos(u + v + t)
                );
                
                u = fmod(u + color.r * 0.1, 6.28318);
                v = fmod(v + color.g * 0.1, 6.28318);
            }
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 최악의 발산 - 체크보드 패턴 (픽셀마다 교차)
const char* g_WorstCaseDivergentShader = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 체크보드 패턴 - 최악의 Warp Divergence!
    // 같은 warp 내에서 픽셀들이 번갈아가며 다른 경로 실행
    int x = int(input.position.x);
    int y = int(input.position.y);
    bool usePathA = ((x + y) % 2) == 0;
    
    if (usePathA) {
        // 경로 A
        for (int i = 0; i < 100; i++) {
            float x = input.uv.x * 4.0 - 2.0;
            float y = input.uv.y * 4.0 - 2.0;
            
            float zx = 0.0;
            float zy = 0.0;
            
            for (int j = 0; j < 50; j++) {
                float xtemp = zx * zx - zy * zy + x;
                zy = 2.0 * zx * zy + y;
                zx = xtemp;
                
                if (zx * zx + zy * zy > 4.0) break;
                
                color += float3(
                    sin(zx * 3.14159) * cos(zy * 2.71828),
                    cos(zx * 2.71828) * sin(zy * 3.14159),
                    sin(zx * zy * 1.41421)
                );
            }
        }
    } else {
        // 경로 B
        for (int i = 0; i < 100; i++) {
            float u = input.uv.x * 6.28318;
            float v = input.uv.y * 6.28318;
            
            for (int j = 0; j < 50; j++) {
                float t = float(j) * 0.1;
                
                color += float3(
                    sin(u * cos(v + t)) * cos(u * sin(v - t)),
                    cos(v * sin(u + t)) * sin(v * cos(u - t)),
                    sin(u * v * t) * cos(u + v + t)
                );
                
                u = fmod(u + color.r * 0.1, 6.28318);
                v = fmod(v + color.g * 0.1, 6.28318);
            }
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 랜덤 발산 패턴 추가
const char* g_RandomDivergentShader = R"(
cbuffer Constants : register(b0) {
    int branchMode;
    float threshold;
    float2 padding;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float hash(float2 p) {
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(PSInput input) : SV_TARGET {
    float3 color = float3(0, 0, 0);
    
    // 완전 랜덤 분기 - Warp 내 완전 무작위
    float randVal = hash(input.position.xy);
    bool usePathA = randVal < 0.5;
    
    if (usePathA) {
        // 경로 A
        for (int i = 0; i < 100; i++) {
            float x = input.uv.x * 4.0 - 2.0;
            float y = input.uv.y * 4.0 - 2.0;
            
            float zx = 0.0;
            float zy = 0.0;
            
            for (int j = 0; j < 50; j++) {
                float xtemp = zx * zx - zy * zy + x;
                zy = 2.0 * zx * zy + y;
                zx = xtemp;
                
                if (zx * zx + zy * zy > 4.0) break;
                
                color += float3(
                    sin(zx * 3.14159) * cos(zy * 2.71828),
                    cos(zx * 2.71828) * sin(zy * 3.14159),
                    sin(zx * zy * 1.41421)
                );
            }
        }
    } else {
        // 경로 B
        for (int i = 0; i < 100; i++) {
            float u = input.uv.x * 6.28318;
            float v = input.uv.y * 6.28318;
            
            for (int j = 0; j < 50; j++) {
                float t = float(j) * 0.1;
                
                color += float3(
                    sin(u * cos(v + t)) * cos(u * sin(v - t)),
                    cos(v * sin(u + t)) * sin(v * cos(u - t)),
                    sin(u * v * t) * cos(u + v + t)
                );
                
                u = fmod(u + color.r * 0.1, 6.28318);
                v = fmod(v + color.g * 0.1, 6.28318);
            }
        }
    }
    
    return float4(color * 0.01, 1.0);
}
)";

// 간단한 버텍스 셰이더
const char* g_VertexShader = R"(
struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input) {
    PSInput output;
    output.position = float4(input.position, 1.0);
    output.uv = input.uv;
    return output;
}
)";

struct HelloTriangleVertex
{
    float pos[3];
    float uv[2];
};

class ShaderBranchTest
{
private:
    ID3D11Device*           device           = nullptr;
    ID3D11DeviceContext*    context          = nullptr;
    IDXGISwapChain*         swapChain        = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;

    ID3D11VertexShader* vertexShader              = nullptr;
    ID3D11PixelShader*  pixelShaderStaticA        = nullptr;
    ID3D11PixelShader*  pixelShaderStaticB        = nullptr;
    ID3D11PixelShader*  pixelShaderDynamicUniform = nullptr;
    ID3D11PixelShader*  pixelShaderSwitchUniform  = nullptr;
    ID3D11PixelShader*  pixelShaderDivergent      = nullptr;
    ID3D11PixelShader*  pixelShaderWorstCase      = nullptr;
    ID3D11PixelShader*  pixelShaderRandom         = nullptr;

    ID3D11Buffer*      vertexBuffer   = nullptr;
    ID3D11Buffer*      constantBuffer = nullptr;
    ID3D11InputLayout* inputLayout    = nullptr;

    ID3D11Query* queryDisjoint = nullptr;
    ID3D11Query* queryStart    = nullptr;
    ID3D11Query* queryEnd      = nullptr;

public:
    bool Initialize(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount          = 1;
        sd.BufferDesc.Width     = 800;
        sd.BufferDesc.Height    = 600;
        sd.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow         = hwnd;
        sd.SampleDesc.Count     = 1;
        sd.Windowed             = TRUE;

        D3D_FEATURE_LEVEL featureLevel;
        HRESULT           hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel, &context);

        if (FAILED(hr)) return false;

        ID3D11Texture2D* backBuffer;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
        backBuffer->Release();

        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query            = D3D11_QUERY_TIMESTAMP_DISJOINT;
        device->CreateQuery(&queryDesc, &queryDisjoint);

        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
        device->CreateQuery(&queryDesc, &queryStart);
        device->CreateQuery(&queryDesc, &queryEnd);

        if (!CompileShaders()) return false;
        CreateVertexBuffer();
        CreateConstantBuffer();

        return true;
    }

    bool CompileShaders()
    {
        ID3DBlob* vsBlob    = nullptr;
        ID3DBlob* psBlob    = nullptr;
        ID3DBlob* errorBlob = nullptr;

        HRESULT hr = D3DCompile(g_VertexShader, strlen(g_VertexShader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                std::cout << "VS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                errorBlob->Release();
            }
            return false;
        }

        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0,    DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
        vsBlob->Release();

        hr = D3DCompile(g_StaticShaderA, strlen(g_StaticShaderA), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderStaticA);
        psBlob->Release();

        hr = D3DCompile(g_StaticShaderB, strlen(g_StaticShaderB), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderStaticB);
        psBlob->Release();

        hr = D3DCompile(g_DynamicUniformShader, strlen(g_DynamicUniformShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderDynamicUniform);
        psBlob->Release();

        hr = D3DCompile(g_SwitchUniformShader, strlen(g_SwitchUniformShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                std::cout << "Switch Shader Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                errorBlob->Release();
            }
            return false;
        }
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderSwitchUniform);
        psBlob->Release();

        hr = D3DCompile(g_DynamicDivergentShader, strlen(g_DynamicDivergentShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderDivergent);
        psBlob->Release();

        hr = D3DCompile(g_WorstCaseDivergentShader, strlen(g_WorstCaseDivergentShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderWorstCase);
        psBlob->Release();

        hr = D3DCompile(g_RandomDivergentShader, strlen(g_RandomDivergentShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) return false;
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShaderRandom);
        psBlob->Release();

        return true;
    }

    void CreateVertexBuffer()
    {
        HelloTriangleVertex vertices[] = {
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
            {  { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            {  { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
            {   { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        };

        D3D11_BUFFER_DESC bd = {};
        bd.Usage             = D3D11_USAGE_DEFAULT;
        bd.ByteWidth         = sizeof(vertices);
        bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = vertices;

        device->CreateBuffer(&bd, &initData, &vertexBuffer);
    }

    void CreateConstantBuffer()
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage             = D3D11_USAGE_DEFAULT;
        bd.ByteWidth         = sizeof(ShaderConstants);
        bd.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        device->CreateBuffer(&bd, nullptr, &constantBuffer);
    }

    double MeasureGPUTime(ID3D11PixelShader* pixelShader, int branchMode = -1, float threshold = 0.5f)
    {
        const int WARMUP_FRAMES = 10;
        const int TEST_FRAMES   = 100;

        for (int i = 0; i < WARMUP_FRAMES; i++)
        {
            Render(pixelShader, branchMode, threshold);
        }

        context->Begin(queryDisjoint);
        context->End(queryStart);

        for (int i = 0; i < TEST_FRAMES; i++)
        {
            Render(pixelShader, branchMode, threshold);
        }

        context->End(queryEnd);
        context->End(queryDisjoint);

        while (context->GetData(queryDisjoint, nullptr, 0, 0) == S_FALSE)
        {
            Sleep(1);
        }

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
        context->GetData(queryDisjoint, &disjointData, sizeof(disjointData), 0);

        UINT64 startTime = 0;
        UINT64 endTime   = 0;
        context->GetData(queryStart, &startTime, sizeof(UINT64), 0);
        context->GetData(queryEnd, &endTime, sizeof(UINT64), 0);

        if (disjointData.Disjoint == FALSE)
        {
            UINT64 delta  = endTime - startTime;
            double timeMS = (delta * 1000.0) / disjointData.Frequency / TEST_FRAMES;
            return timeMS;
        }

        return 0.0;
    }

    void Render(ID3D11PixelShader* pixelShader, int branchMode, float threshold)
    {
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(renderTargetView, clearColor);

        if (branchMode >= 0)
        {
            ShaderConstants constants;
            constants.branchMode = branchMode;
            constants.threshold  = threshold;
            context->UpdateSubresource(constantBuffer, 0, nullptr, &constants, 0, 0);
            context->PSSetConstantBuffers(0, 1, &constantBuffer);
        }

        context->OMSetRenderTargets(1, &renderTargetView, nullptr);
        context->IASetInputLayout(inputLayout);

        UINT stride = sizeof(HelloTriangleVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        context->VSSetShader(vertexShader, nullptr, 0);
        context->PSSetShader(pixelShader, nullptr, 0);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
        context->RSSetViewports(1, &vp);

        context->Draw(4, 0);
        context->Flush();
    }

    void RunTests()
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "   셰이더 분기 성능 테스트" << std::endl;
        std::cout << "========================================\n"
                  << std::endl;

        std::cout << "[1] 정적 분기 (Static Branching)" << std::endl;
        double staticA = MeasureGPUTime(pixelShaderStaticA);
        std::cout << "    경로 A (프랙탈):       " << staticA << " ms" << std::endl;

        double staticB = MeasureGPUTime(pixelShaderStaticB);
        std::cout << "    경로 B (삼각함수):     " << staticB << " ms\n"
                  << std::endl;

        std::cout << "[2] IF 문 균일 분기 (IF Uniform Branching)" << std::endl;
        std::cout << "    - 상수 버퍼로 분기, 모든 픽셀이 같은 경로" << std::endl;
        double uniformA = MeasureGPUTime(pixelShaderDynamicUniform, 0);
        std::cout << "    경로 A:                " << uniformA << " ms" << std::endl;

        double uniformB = MeasureGPUTime(pixelShaderDynamicUniform, 1);
        std::cout << "    경로 B:                " << uniformB << " ms\n"
                  << std::endl;

        std::cout << "[3] SWITCH 문 균일 분기 (SWITCH Uniform Branching)" << std::endl;
        std::cout << "    - 상수 버퍼로 분기, 모든 픽셀이 같은 경로" << std::endl;
        double switchA = MeasureGPUTime(pixelShaderSwitchUniform, 0);
        std::cout << "    경로 A:                " << switchA << " ms" << std::endl;

        double switchB = MeasureGPUTime(pixelShaderSwitchUniform, 1);
        std::cout << "    경로 B:                " << switchB << " ms\n"
                  << std::endl;

        std::cout << "[4] 동적 발산 분기 (Dynamic Divergent Branching)" << std::endl;
        std::cout << "    - 픽셀마다 다른 경로 (Warp Divergence 발생!)" << std::endl;
        double divergent = MeasureGPUTime(pixelShaderDivergent, 0, 0.5f);
        std::cout << "    혼합 실행 (50/50):     " << divergent << " ms\n"
                  << std::endl;

        std::cout << "[5] 체크보드 발산 (Checkerboard Divergence)" << std::endl;
        std::cout << "    - 픽셀마다 교차하는 최악의 패턴" << std::endl;
        double worstCase = MeasureGPUTime(pixelShaderWorstCase, 0, 0.5f);
        std::cout << "    체크보드 실행:         " << worstCase << " ms\n"
                  << std::endl;

        std::cout << "[6] 랜덤 발산 (Random Divergence)" << std::endl;
        std::cout << "    - 완전 무작위 분기" << std::endl;
        double randomDiv = MeasureGPUTime(pixelShaderRandom, 0, 0.5f);
        std::cout << "    랜덤 실행:             " << randomDiv << " ms\n"
                  << std::endl;

        std::cout << "========================================" << std::endl;
        std::cout << "   분석 결과" << std::endl;
        std::cout << "========================================\n"
                  << std::endl;

        double avgStatic  = (staticA + staticB) / 2.0;
        double avgUniform = (uniformA + uniformB) / 2.0;
        double avgSwitch  = (switchA + switchB) / 2.0;

        std::cout << "정적 분기 평균:           " << avgStatic << " ms" << std::endl;
        std::cout << "IF 문 균일 분기 평균:     " << avgUniform << " ms" << std::endl;
        std::cout << "SWITCH 문 균일 분기 평균: " << avgSwitch << " ms" << std::endl;
        std::cout << "동적 발산 분기:           " << divergent << " ms\n"
                  << std::endl;

        double uniformOverhead  = ((avgUniform - avgStatic) / avgStatic) * 100.0;
        double switchOverhead   = ((avgSwitch - avgStatic) / avgStatic) * 100.0;
        double divergentPenalty = ((divergent - avgStatic) / avgStatic) * 100.0;

        std::cout << "IF 문 오버헤드:           " << uniformOverhead << "%" << std::endl;
        std::cout << "SWITCH 문 오버헤드:       " << switchOverhead << "%" << std::endl;
        std::cout << "발산 분기 페널티:         " << divergentPenalty << "%\n"
                  << std::endl;

        double worstPenalty  = ((worstCase - avgStatic) / avgStatic) * 100.0;
        double randomPenalty = ((randomDiv - avgStatic) / avgStatic) * 100.0;

        std::cout << "체크보드 발산 페널티:     " << worstPenalty << "%" << std::endl;
        std::cout << "랜덤 발산 페널티:         " << randomPenalty << "%" << std::endl;

        std::cout << "\n========================================" << std::endl;
        std::cout << "   결론" << std::endl;
        std::cout << "========================================\n"
                  << std::endl;

        if (abs(uniformOverhead) < 5.0 && abs(switchOverhead) < 5.0)
        {
            std::cout << "✓ IF/SWITCH 균일 분기: 정적 분기와 성능 차이 없음 (최적화됨)" << std::endl;

            double ifVsSwitch = abs(avgSwitch - avgUniform);
            if (ifVsSwitch < 0.1)
            {
                std::cout << "✓ IF vs SWITCH: 컴파일러가 동일하게 최적화함 (차이: " << ifVsSwitch << "ms)" << std::endl;
            }
            else
            {
                std::cout << "  IF vs SWITCH 차이: " << ((avgSwitch - avgUniform) / avgUniform * 100.0) << "%" << std::endl;
            }
        }
        else
        {
            std::cout << "✗ IF 문: 정적 분기 대비 " << uniformOverhead << "% 차이" << std::endl;
            std::cout << "✗ SWITCH 문: 정적 분기 대비 " << switchOverhead << "% 차이" << std::endl;
        }

        if (divergentPenalty > 10.0)
        {
            std::cout << "\n✗ 발산 분기: 심각한 성능 저하 발생!" << std::endl;
            std::cout << "  → Warp 내 스레드들이 서로 다른 경로를 실행하여" << std::endl;
            std::cout << "    직렬화(serialization)가 발생했습니다." << std::endl;
        }
        else
        {
            std::cout << "\n✓ 발산 분기: 예상보다 낮은 페널티" << std::endl;
            std::cout << "  → GPU가 분기를 효율적으로 처리했을 가능성" << std::endl;
        }

        std::cout << "\n========================================" << std::endl;
    }

    void Cleanup()
    {
        if (queryStart) queryStart->Release();
        if (queryEnd) queryEnd->Release();
        if (queryDisjoint) queryDisjoint->Release();
        if (constantBuffer) constantBuffer->Release();
        if (vertexBuffer) vertexBuffer->Release();
        if (inputLayout) inputLayout->Release();
        if (pixelShaderDivergent) pixelShaderDivergent->Release();
        if (pixelShaderDynamicUniform) pixelShaderDynamicUniform->Release();
        if (pixelShaderSwitchUniform) pixelShaderSwitchUniform->Release();
        if (pixelShaderStaticB) pixelShaderStaticB->Release();
        if (pixelShaderStaticA) pixelShaderStaticA->Release();
        if (vertexShader) vertexShader->Release();
        if (renderTargetView) renderTargetView->Release();
        if (swapChain) swapChain->Release();
        if (pixelShaderWorstCase) pixelShaderWorstCase->Release();
        if (pixelShaderRandom) pixelShaderRandom->Release();
        if (context) context->Release();
        if (device) device->Release();
    }
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main()
{
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ShaderBranchTest";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, L"ShaderBranchTest", L"Shader Branch Performance Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd)
    {
        std::cout << "윈도우 생성 실패" << std::endl;
        return -1;
    }

    ShowWindow(hwnd, SW_HIDE);

    ShaderBranchTest test;
    if (!test.Initialize(hwnd))
    {
        std::cout << "초기화 실패" << std::endl;
        return -1;
    }

    std::cout << "GPU 타이머를 이용한 정확한 성능 측정 중..." << std::endl;
    test.RunTests();
    test.Cleanup();

    DestroyWindow(hwnd);
    return 0;
}