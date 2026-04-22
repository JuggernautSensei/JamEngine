#include "pch.h"
#include "DebugRenderer.h"

#include "Graphics.h"
#include "ShaderCompiler.h"

namespace Jam
{

namespace
{
    constexpr StringView kDebugRendererShaderSource = R"(
    struct VSInput { float3 pos : POSITION; uint4 col : COLOR; };
    struct PSInput { float4 pos : SV_POSITION; float4 col : COLOR; };
    cbuffer Constants : register(b0) { float4x4 viewProj; float screenWidth; float screenHeight; float padding0; float padding1; };

    PSInput VSMain(VSInput input)
    {
        PSInput output;
    #ifdef USE_VIEW_PROJ
        output.pos = mul(float4(input.pos, 1.0f), viewProj);
    #else
        output.pos = float4(input.pos.xy, 0.f, 1.f);
        output.pos.x /= screenWidth;
        output.pos.x = output.pos.x * 2.f - 1.f;
        output.pos.y /= screenHeight;
        output.pos.y = 1.f - output.pos.y * 2.f;
    #endif
        output.col = input.col; 
        return output;
    }

    float4 PSMain(PSInput input) : SV_TARGET
    {
        return input.col;
    }
    )";

}   // namespace

DebugRenderer::DebugRenderer()
{
    Graphics& gfx = GetGraphics();

    ShaderCompilerInfo psi = { eShader::PixelShader, "PSMain", {}, eShaderCompileFlags::Default };

    {
        ShaderMacro macro[] = {
            { "USE_VIEW_PROJ", "" }
        };

        ShaderCompilerInfo vsi = { eShader::VertexShader, "VSMain", macro, eShaderCompileFlags::Default };
        ShaderCompiler     vsc = ShaderCompiler::CompileHLSLSource(kDebugRendererShaderSource, vsi);
        JAM_ASSERT(vsc.IsSucceeded(), "Failed to compile debug renderer world vertex shader: {}", vsc.GetErrorMessage());
        ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        ShaderCompiler psc = ShaderCompiler::CompileHLSLSource(kDebugRendererShaderSource, psi);
        JAM_ASSERT(psc.IsSucceeded(), "Failed to compile debug renderer pixel shader: {}", psc.GetErrorMessage());
        ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_worldPh = gfx.CreateProgram(vsh, psh, true);
    }

    {
        ShaderCompilerInfo vsi = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::Default };
        ShaderCompiler     vsc = ShaderCompiler::CompileHLSLSource(kDebugRendererShaderSource, vsi);
        JAM_ASSERT(vsc.IsSucceeded(), "Failed to compile debug renderer screen vertex shader: {}", vsc.GetErrorMessage());
        ShaderHandle vsh = gfx.CreateShader(vsc.GetBytecode());

        ShaderCompiler psc = ShaderCompiler::CompileHLSLSource(kDebugRendererShaderSource, psi);
        JAM_ASSERT(psc.IsSucceeded(), "Failed to compile debug renderer pixel shader: {}", psc.GetErrorMessage());
        ShaderHandle psh = gfx.CreateShader(psc.GetBytecode());

        m_screenPh = gfx.CreateProgram(vsh, psh, true);
    }

    m_cbh = gfx.CreateConstantBuffer(sizeof(Constants));
}

DebugRenderer::~DebugRenderer()
{
    JAM_DESTROY_PROGRAM(m_worldPh);
    JAM_DESTROY_PROGRAM(m_screenPh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
    JAM_DESTROY_VERTEX_BUFFER(m_vbh);
    JAM_DESTROY_INDEX_BUFFER(m_ibh);
}

void DebugRenderer::Begin(
    const RenderView& _view)
{
    m_bBeginDraw = true;

    Graphics& gfx   = GetGraphics();
    Constants cb    = {};
    cb.viewProj     = (_view.view * _view.proj).Transpose();
    cb.screenWidth  = static_cast<float>(_view.vp.w);
    cb.screenHeight = static_cast<float>(_view.vp.h);
    gfx.UpdateBuffer(m_cbh, AsMemoryView(cb));
}

void DebugRenderer::End()
{
    JAM_ASSERT(m_bBeginDraw, "End called without matching Begin.");

    Flush();

    m_numIndices  = 0;
    m_numVerts    = 0;
    m_cursorPos   = Vec3::kZero;
    m_cursorCol   = RGBA::kBlack;
    m_bCursorDown = false;
    m_bBeginDraw  = false;
    m_drawSpace   = eDebugRendererSpace::World;
}

void DebugRenderer::SetColor(
    const RGBA _col)
{
    m_cursorCol = _col;
}

void DebugRenderer::SetSpace(
    const eDebugRendererSpace _space)
{
    m_drawSpace = _space;
}

void DebugRenderer::SetRenderFlags(
    const Flags<eRenderFlags> _flags)
{
    m_userRenderFlags = _flags;
}

void DebugRenderer::MoveTo(
    const Vec3 _pos)
{
    m_bCursorDown = false;
    m_cursorPos   = _pos;
}

void DebugRenderer::MoveTo(float _x, float _y, float _z)
{
    MoveTo({ _x, _y, _z });
}

void DebugRenderer::DrawTo(
    const Vec3 _pos)
{
    if (!m_bCursorDown)
    {
        m_bCursorDown = true;
        m_cursorPos   = _pos;
        return;
    }

    if (m_numVerts >= kMaxNumVerts - 2 || m_numIndices >= kMaxNumIndices - 2)
    {
        Flush();
    }

    const Int32 i0 = m_numVerts++;
    const Int32 i1 = m_numVerts++;

    m_cpuVerts[i0]               = { m_cursorPos, m_cursorCol };
    m_cursorPos                  = _pos;
    m_cpuVerts[i1]               = { m_cursorPos, m_cursorCol };
    m_cpuIndices[m_numIndices++] = static_cast<UInt16>(i0);
    m_cpuIndices[m_numIndices++] = static_cast<UInt16>(i1);
}

void DebugRenderer::DrawTo(float _x, float _y, float _z)
{
    DrawTo({ _x, _y, _z });
}

void DebugRenderer::Flush()
{
    JAM_ASSERT(m_bBeginDraw, "Flush called without matching Begin.");

    if (m_numIndices == 0)
    {
        return;
    }

    Graphics& gfx = GetGraphics();

    constexpr Int32 kVStride = static_cast<Int32>(sizeof(Vertex));
    constexpr Int32 kIStride = static_cast<Int32>(sizeof(UInt16));

    const Int32 needVbhByteWidth = m_numVerts * kVStride;
    if (m_vbhByteWidth < needVbhByteWidth)
    {
        JAM_DESTROY_VERTEX_BUFFER(m_vbh);
        m_vbhByteWidth = needVbhByteWidth + 256 * kVStride;
        m_vbh          = gfx.CreateDynamicVertexBuffer(m_vbhByteWidth, Vertex::s_layout);
    }

    const Int32 needIbhByteWidth = m_numIndices * kIStride;
    if (m_ibhByteWidth < needIbhByteWidth)
    {
        JAM_DESTROY_INDEX_BUFFER(m_ibh);
        m_ibhByteWidth = needIbhByteWidth + 256 * kIStride;
        m_ibh          = gfx.CreateDynamicIndexBuffer(m_ibhByteWidth);
    }

    gfx.UpdateBuffer(m_vbh, { m_cpuVerts.data(), m_numVerts * kVStride });
    gfx.UpdateBuffer(m_ibh, { m_cpuIndices.data(), m_numIndices * kIStride });

    gfx.SetProgram(m_drawSpace == eDebugRendererSpace::Screen ? m_screenPh : m_worldPh);
    gfx.SetBlendFlags(eBlendFlags::WriteMask_All, 0);
    gfx.SetRenderFlags(eRenderFlags::LineAA | eRenderFlags::Primitive_LineList | m_userRenderFlags);
    gfx.SetConstantBuffer(m_cbh, eShader::VertexShader, 0);

    gfx.SetVertexBuffer(m_vbh, m_numVerts, 0);
    gfx.SetIndexBuffer(m_ibh, 0, m_numIndices);
    gfx.Submit();

    m_numVerts   = 0;
    m_numIndices = 0;
}

}   // namespace Jam