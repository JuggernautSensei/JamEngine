#include "pch.h"

#include "TriangleScene.h"

// 드로우할 데이터	-> VertexHandle IndexHandle
// GPU 프로그램		-> ProgramHandle

struct MyVertex
{
    Vec2 pos;
    RGBA color;
};

struct CB_CAMERA
{
    Vec3  offset;
    float dummy;
};

const char* k_source = R"(
struct VS_INPUT
{
    float2 pos : POSITION;
    uint4 color : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer CB_CAMERA : register(b0)
{
    float3 offset;
    float   dummy;
};

PS_INPUT VSMain(VS_INPUT _input)
{
    PS_INPUT output;

    output.pos = float4(_input.pos, 0.f,1.f);
    output.pos.x += offset.x;
    output.pos.y += offset.y;
    output.color.r = _input.color.r / 255.f;
    output.color.g = _input.color.g / 255.f;
    output.color.b = _input.color.b / 255.f;
    output.color.a = 1.f;
    return output;
}

float4 PSMain(PS_INPUT _input) : SV_TARGET
{
    return _input.color;
}

)";

void TriangleScene::OnEnter()
{
    VertexLayout layout;
    layout.Add(eVertexAttribute::Position, eAttributeFormat::Float2);
    layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);

    // 버텍스 버퍼
    MyVertex verts[3] = {
        MyVertex {  { 0.f, 0.5f }, RGBA { 255, 0, 0 } },
        MyVertex {  { 0.5f, 0.f }, RGBA { 0, 255, 0 } },
        MyVertex { { -0.5f, 0.f }, RGBA { 0, 0, 255 } },
    };
    m_vbh = Graphics::CreateVertexBuffer(MemoryView { verts }, layout);

    // 인덱스 버퍼
    UInt16 indices[3] = { 0, 1, 2 };
    m_ibh             = Graphics::CreateIndexBuffer(MemoryView { indices }, eIndexFormat::U16);

    // 쉐이더 핸들
    {
        // vertex shader
        ShaderCompiler compiler;
        eErrorCode     ec = compiler.CompileFromSource(k_source, eShader::VertexShader, "VSMain");
        if (IsError(ec))
        {
            Log::Error("shader compile failed. {}", GetErrorMessage());
            JAM_ASSERT(0, "shader compile failed");
        }
        ShaderHandle vsh = Graphics::CreateShader(compiler.GetBytecode());

        // pixel shader
        ec = compiler.CompileFromSource(k_source, eShader::PixelShader, "PSMain");
        if (IsError(ec))
        {
            Log::Error("shader compile failed. {}", GetErrorMessage());
            JAM_ASSERT(0, "shader compile failed");
        }
        ShaderHandle psh = Graphics::CreateShader(compiler.GetBytecode());

        m_program = Graphics::CreateProgram(vsh, psh, true);
    }

    m_cbh = Graphics::CreateConstantBuffer(sizeof(CB_CAMERA));
}

void TriangleScene::OnExit()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_PROGRAM(m_program);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
}

void TriangleScene::OnRender(const RenderView& _view)
{
    Graphics::SetViewport(_view.viewport);
    Graphics::SetFrameBuffer(_view.frameBufferHandle);
    Graphics::ClearRenderTarget(_view.frameBufferHandle, RGBA::k_black);

    Graphics::SetVertexBuffer(m_vbh);
    Graphics::SetIndexBuffer(m_ibh);
    Graphics::SetProgram(m_program);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);

    CB_CAMERA cb;
    cb.offset = m_offset;
    Graphics::UploadBuffer(m_cbh, MemoryView { cb });

    Graphics::Submit();   // gpu 제출한다.
}

void TriangleScene::OnImGuiRender()
{
    ImGui::Begin("hello world");
    ImGui::Text("hello my name is triangle scene");

    ImGui::SliderFloat3("translation", static_cast<float*>(&m_offset.x), -3.f, 3.f);
    ImGui::End();
}