#include "pch.h"

#include "TriangleScene.h"

// 드로우할 데이터	-> VertexHandle IndexHandle
// GPU 프로그램		-> ProgramHandle

struct MyVertex
{
    Vec2 pos;
    RGBA color;
};

struct MyCAMERA
{
    Vec3  offset;
    float dummy;
};

const char* k_source = R"(
struct VS_INPUT
{
    float2 pos : POSITION;
    uint4 radiance : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 radiance : COLOR;
};

cbuffer MyCAMERA : register(b0)
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
    output.radiance.r = _input.radiance.r / 255.f;
    output.radiance.g = _input.radiance.g / 255.f;
    output.radiance.b = _input.radiance.b / 255.f;
    output.radiance.a = 1.f;
    return output;
}

float4 PSMain(PS_INPUT _input) : SV_TARGET
{
    return _input.radiance;
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
    m_vbh = Graphics::CreateVertexBuffer({ verts }, layout);

    // 인덱스 버퍼
    UInt16 indices[3] = { 0, 1, 2 };
    m_ibh             = Graphics::CreateIndexBuffer({ indices }, eIndexFormat::U16);

    // 쉐이더 핸들
    {
        // vertex shader
        ShaderCompiler compiler;
        if (!compiler.CompileFromSource(k_source, eShader::VertexShader, "VSMain"))
        {
            Log::Error("shader compile failed. {}", GetErrorString());
            JAM_ASSERT(0, "shader compile failed");
        }
        ShaderHandle vsh = Graphics::CreateShader(compiler.GetBytecode());

        // pixel shader
        if (!compiler.CompileFromSource(k_source, eShader::PixelShader, "PSMain"))
        {
            Log::Error("shader compile failed. {}", GetErrorString());
            JAM_ASSERT(0, "shader compile failed");
        }
        ShaderHandle psh = Graphics::CreateShader(compiler.GetBytecode());

        m_program = Graphics::CreateProgram(vsh, psh, true);
    }

    m_cbh = Graphics::CreateConstantBuffer(sizeof(MyCAMERA));
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
    Graphics::SetViewport(_view.vp);
    Graphics::SetFrameBuffer(_view.fbh);
    Graphics::ClearRenderTarget(_view.fbh, RGBA::k_black);

    Graphics::SetGfxState(RenderState::Default());
    Graphics::SetVertexBuffer(m_vbh);
    Graphics::SetIndexBuffer(m_ibh);
    Graphics::SetProgram(m_program);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);

    MyCAMERA cb;
    cb.offset = m_offset;
    Graphics::UploadBuffer(m_cbh, { cb });

    Graphics::Submit();   // gpu 제출한다.
}

void TriangleScene::OnImGuiRender()
{
    ImGui::Begin("hello world");
    ImGui::Text("hello my name is triangle scene");

    ImGui::SliderFloat3("translation", static_cast<float*>(&m_offset.x), -3.f, 3.f);
    ImGui::End();
}
