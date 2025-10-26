#include "pch.h"

#include "HelloTriangleScene.h"

struct HelloTriangleVertex
{
    Vec2 position = Vec2::Zero;
    RGBA color    = RGBA::k_white;

    static VertexLayout s_layout;
    static void         InitLayout()
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float2);
        layout.Add(eVertexAttribute::Color0, eAttributeFormat::UInt8x4);
        s_layout = std::move(layout);
    }
};
VertexLayout HelloTriangleVertex::s_layout = {};

constexpr const char* k_source = R"(
struct VSInput
{
    float2 position : POSITION;
    uint4  color    : COLOR0;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.color = float4(input.color) / 255.0f;
    return output;
}
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

HelloTriangleScene::HelloTriangleScene()
{
    HelloTriangleVertex::InitLayout();

    ShaderCompiler compiler;

    // vertex shader
    eErrorCode ec;
    ec = compiler.CompileFromSource(k_source, eShader::VertexShader, "VSMain");
    JAM_ASSERT(IsOk(ec), "vertex shader compile failed");
    ShaderHandle vsh = Graphics::CreateShader(compiler.GetBytecode());

    // pixel shader
    ec = compiler.CompileFromSource(k_source, eShader::PixelShader, "PSMain");
    JAM_ASSERT(IsOk(ec), "pixel shader compile failed");
    ShaderHandle psh = Graphics::CreateShader(compiler.GetBytecode());
    m_program        = Graphics::CreateProgram(vsh, psh, true);

    // vertex buffer
    HelloTriangleVertex vertices[] = {
        {   Vec2 { 0.0f, 0.5f }, RGBA(255,   0,   0) },
        {  Vec2 { 0.5f, -0.5f },   RGBA(0, 255,   0) },
        { Vec2 { -0.5f, -0.5f },   RGBA(0,   0, 255) },
    };

    // index buffer
    UInt16 indices[] = { 0, 1, 2 };

    m_vbh = Graphics::CreateVertexBuffer(MemoryView { vertices }, HelloTriangleVertex::s_layout);
    m_ibh = Graphics::CreateIndexBuffer(MemoryView { indices });
}

HelloTriangleScene::~HelloTriangleScene()
{
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
    JAM_DESTROY_PROGRAM(m_program);
}

void HelloTriangleScene::OnRender(const SceneView& _view)
{
    Graphics::SetFrameBuffer(_view.frameBufferHandle);
    Graphics::ClearRenderTarget(_view.frameBufferHandle, RGBA::k_black);
    Graphics::SetViewport(_view.viewport);

    Graphics::SetProgram(m_program);
    Graphics::SetVertexBuffer(m_vbh);
    Graphics::SetIndexBuffer(m_ibh);
    Graphics::Submit();
}