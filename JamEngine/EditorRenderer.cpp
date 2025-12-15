#include "pch.h"

#include "EditorRenderer.h"
#include "Graphics.h"
#include "MeshFactory.h"
#include "RenderView.h"
#include "ShaderCompiler.h"

namespace jam
{

struct CB_EDITOR_RENDERER
{
    Mat4   viewProj = Mat4::Identity;
    Mat4   mtx      = Mat4::Identity;
    Vec3   eye      = Vec3::Zero;
    UInt32 flags    = 0;
    Vec4   color    = Vec4::One;
};

//////////////////////////////////
///      GRID SHADER          ///
//////////////////////////////////

constexpr const char* k_editorRendererGridShaderSource = R"(
static const float4 k_axisXCol = float4(253 / 255.f, 54 / 255.f, 83 / 255.f, 1.f);
static const float4 k_axisZCol = float4(48 / 255.f, 128 / 255.f, 219 / 255.f, 1.f);
static const float4 k_minorGridCol = float4(140.f / 255.f, 140.f / 255.f, 140.f / 255.f, 1.f);
static const float4 k_majorGridCol = k_minorGridCol * 0.9f;
static const float k_gridCellSize = 1.f;
static const float k_gridFadeStart = 80.f;
static const float k_gridFadeEnd = 150.f;
static const float k_minorGridLineThickness = 1.f;
static const float k_majorGridLineThickness = 2.f;
static const float k_axisLineThickness = 2.f;
static const float k_viewAngleFadeStart = 0.0f;  
static const float k_viewAngleFadeEnd = 0.20f;   

#define DRAW_GRID_FLAG 1
#define DRAW_AXIS_FLAG 2

struct VS_INPUT
{
    float3 posL : POSITION;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
};

cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3 eye;
    uint flags;
    float4 color;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posW = input.posL;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    bool bDrawGrid = (flags & DRAW_GRID_FLAG) != 0;
    bool bDrawAxis = (flags & DRAW_AXIS_FLAG) != 0;
    
    float2 gridCoord = input.posW.xz / k_gridCellSize;
    float2 fracCoord = frac(gridCoord);
    float2 distToLine = min(fracCoord, 1.0 - fracCoord);
    
    float2 dx_grid = ddx(gridCoord);
    float2 dy_grid = ddy(gridCoord);
    float2 derivatives = float2(length(float2(dx_grid.x, dy_grid.x)), length(float2(dx_grid.y, dy_grid.y)));
    
    float2 lineAA = distToLine / derivatives;
    float2 grid = smoothstep(0.0, k_minorGridLineThickness, lineAA);
    float minGrid = min(grid.x, grid.y);
    
    float2 majorGridCoord = input.posW.xz / (k_gridCellSize * 10.0);
    float2 majorFracCoord = frac(majorGridCoord);
    float2 majorDistToLine = min(majorFracCoord, 1.0 - majorFracCoord);
    float2 majorDx = ddx(majorGridCoord);
    float2 majorDy = ddy(majorGridCoord);
    float2 majorDerivatives = float2(length(float2(majorDx.x, majorDy.x)), length(float2(majorDx.y, majorDy.y)));
    float2 majorLineAA = majorDistToLine / majorDerivatives;
    float2 majorGrid = smoothstep(0.0, k_majorGridLineThickness, majorLineAA);
    float minMajorGrid = min(majorGrid.x, majorGrid.y);
    
    // 거리 기반 페이드
    float dist = distance(input.posW, eye);
    float t = 1.0 - smoothstep(k_gridFadeStart, k_gridFadeEnd, dist);
    float distanceFade = t * t * t;
    
    // 시선 각도 기반 페이드
    float3 viewDir = normalize(eye - input.posW);
    float3 gridNormal = float3(0.0, 1.0, 0.0); 
    float viewDotNormal = abs(dot(viewDir, gridNormal));
    float viewAngleFade = smoothstep(k_viewAngleFadeStart, k_viewAngleFadeEnd, viewDotNormal);
    
    // 최종 페이드
    float fadeFactor = distanceFade * viewAngleFade;

    float xAxis = smoothstep(0.0, k_axisLineThickness, abs(input.posW.z) / derivatives.y);
    float zAxis = smoothstep(0.0, k_axisLineThickness, abs(input.posW.x) / derivatives.x);
    
    float xAxisAlpha = 1.0 - xAxis;
    float zAxisAlpha = 1.0 - zAxis;
    float majorGridAlpha = 1.0 - minMajorGrid;
    float minorGridAlpha = 1.0 - minGrid;
    
    float4 outColor = float4(0.0, 0.0, 0.0, 0.0);
    if (bDrawAxis && xAxisAlpha > 0.01)
    {
        outColor = k_axisXCol;
        outColor.a = xAxisAlpha;
    }
    else if (bDrawAxis && zAxisAlpha > 0.01)
    {
        outColor = k_axisZCol;
        outColor.a = zAxisAlpha;
    }
    else if (bDrawGrid && majorGridAlpha > 0.01)
    {
        outColor = k_majorGridCol;
        outColor.a = majorGridAlpha;
    }
    else if (bDrawGrid && minorGridAlpha > 0.01)
    {
        outColor = k_minorGridCol;
        outColor.a = minorGridAlpha;
    }
    
    outColor.a *= fadeFactor;
    return outColor;
}
)";

//////////////////////////////////
///      LINE SHADER          ///
//////////////////////////////////

constexpr const char* k_editorRendererLineShaderSource = R"(
static const float k_depthBias = 0.0001f;

cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3   eye;
    float    pad;
    float4   color;
};

struct VS_INPUT
{
    float3 posL : POSITION;
    uint4 color : COLOR;
};

struct PS_INPUT
{
    float4 posH  : SV_POSITION;
    float4 color : COLOR;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    float4 posW = mul(float4(input.posL, 1.0f), mtx);
    output.posH = mul(posW, viewProj);
    output.posH.z -= k_depthBias * output.posH.w;
    output.color = float4(
        input.color.r / 255.0f,
        input.color.g / 255.0f,
        input.color.b / 255.0f,
        input.color.a / 255.0f
    );
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.color;
}

)";

//////////////////////////////////
///    FILLED SHADER          ///
//////////////////////////////////

constexpr const char* k_editorRendererFilledShaderSource = R"(
cbuffer CB_EDITOR_RENDERER : register(b0)
{
    float4x4 viewProj;
    float4x4 mtx;
    float3 eye;
    float pad;
    float4 color;
};

struct VS_INPUT
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.posW = mul(float4(input.posL, 1.0f), mtx).xyz;
    output.normalW = mul(float4(input.normalL, 0.0f), mtx).xyz;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    // Simple lighting
    float3 normal = normalize(input.normalW);
    float3 lightDir = normalize(float3(1, 2, 1));
    float ndotl = max(0.0, dot(normal, lightDir));
    float3 lighting = color.rgb * (ndotl * 0.7 + 0.3);
    return float4(lighting, color.a);
}
)";

}   // namespace jam

namespace jam
{

// define static vertexLayout
VertexLayout EditorRenderer::LineVertex::s_layout;

void EditorRenderer::Init()
{
    LineVertex::Init();

    // constant buffer
    m_cbh = Graphics::CreateConstantBuffer(sizeof(CB_EDITOR_RENDERER));
    Graphics::SetConstantBufferName(m_cbh, "EditorRenderer_ConstantBuffer");

    ShaderCompiler compiler;

    // Grid shaders
    {
        if (!compiler.CompileSource(k_editorRendererGridShaderSource, eShader::VertexShader, "VSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor grid vertex shader compilation failed");
        ShaderHandle gridVsh = Graphics::CreateShader(compiler.GetBytecode());

        if (!compiler.CompileSource(k_editorRendererGridShaderSource, eShader::PixelShader, "PSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor grid pixel shader compilation failed");
        ShaderHandle gridPsh = Graphics::CreateShader(compiler.GetBytecode());

        m_gridProgramHandle = Graphics::CreateProgram(gridVsh, gridPsh, true);
    }

    // Line shaders
    {
        if (!compiler.CompileSource(k_editorRendererLineShaderSource, eShader::VertexShader, "VSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor line vertex shader compilation failed");
        ShaderHandle lineVsh = Graphics::CreateShader(compiler.GetBytecode());

        if (!compiler.CompileSource(k_editorRendererLineShaderSource, eShader::PixelShader, "PSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor line pixel shader compilation failed");
        ShaderHandle linePsh = Graphics::CreateShader(compiler.GetBytecode());

        m_lineProgramHandle = Graphics::CreateProgram(lineVsh, linePsh, true);
    }

    // Filled shaders
    {
        if (!compiler.CompileSource(k_editorRendererFilledShaderSource, eShader::VertexShader, "VSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor filled vertex shader compilation failed");
        ShaderHandle filledVsh = Graphics::CreateShader(compiler.GetBytecode());

        if (!compiler.CompileSource(k_editorRendererFilledShaderSource, eShader::PixelShader, "PSMain", eShaderCompileFlags_DebugDefault))
            JAM_ASSERT(0, "Editor filled pixel shader compilation failed");
        ShaderHandle filledPsh = Graphics::CreateShader(compiler.GetBytecode());

        m_filledProgramHandle = Graphics::CreateProgram(filledVsh, filledPsh, true);
    }

    // 라인 버퍼 생성
    m_lineVbh = Graphics::CreateDynamicVertexBuffer(k_lineLimit * 2, LineVertex::s_layout);
    m_lineIbh = Graphics::CreateDynamicIndexBuffer(k_lineLimit * 2);

    // 그리드 쿼드 생성
    CreateGridQuad_();

    // 프리미티브 생성
    CreatePrimitives_();

    // gfx state
    m_lineState           = RenderState::k_default;
    m_lineState.primitive = ePrimitive::LineList;
    m_lineState.bLineAA   = true;

    m_gridState               = RenderState::k_depthTest;
    m_gridState.depthTestFunc = eCompareFunc::LessEqual;
    m_gridState.cullMode      = eCullMode::None;
    m_gridState.primitive     = ePrimitive::TriangleList;

    m_filledState               = RenderState::k_depthTest;
    m_filledState.depthTestFunc = eCompareFunc::LessEqual;
    m_filledState.cullMode      = eCullMode::Back;
    m_filledState.primitive     = ePrimitive::TriangleList;

    // blend
    m_blendState = BlendState::k_alphaBlend;
}

void EditorRenderer::Shutdown()
{
    JAM_DESTROY_VERTEX_BUFFER(m_gridVbh);
    JAM_DESTROY_INDEX_BUFFER(m_gridIbh);
    JAM_DESTROY_VERTEX_BUFFER(m_lineVbh);
    JAM_DESTROY_INDEX_BUFFER(m_lineIbh);
    JAM_DESTROY_CONSTANT_BUFFER(m_cbh);
    JAM_DESTROY_PROGRAM(m_gridProgramHandle);
    JAM_DESTROY_PROGRAM(m_lineProgramHandle);
    JAM_DESTROY_PROGRAM(m_filledProgramHandle);

    m_cube.Unload();
    m_sphere.Unload();
    m_cylinder.Unload();
}

void EditorRenderer::CreateGridQuad_()
{
    constexpr float k_gridSize = 10000.0f;
    // todo: this is not infinite grid. grid cube need move with camera position

    struct GridVertex
    {
        Vec3 pos;
    };

    VertexLayout vl;
    vl.Add(eVertexAttribute::Position, eAttributeFormat::Float3);

    constexpr GridVertex verts[4] = {
        { Vec3(-k_gridSize, 0.0f, -k_gridSize) },
        { Vec3(-k_gridSize, 0.0f, k_gridSize) },
        { Vec3(k_gridSize, 0.0f, k_gridSize) },
        { Vec3(k_gridSize, 0.0f, -k_gridSize) }
    };

    constexpr UInt16 k_indices[6] = { 0, 1, 2, 0, 2, 3 };

    m_gridVbh = Graphics::CreateVertexBuffer(verts, vl);
    m_gridIbh = Graphics::CreateIndexBuffer(k_indices);
}

void EditorRenderer::CreatePrimitives_()
{
    VertexLayout vl;
    vl.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
    vl.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);

    m_sphere   = CreateSphereMesh(1.0f, 4, vl);
    m_cylinder = CreateCylinder(1.0f, 1.0f, 32, vl);
    m_cube     = CreateCubeMesh(Vec3::One, vl);
}

void EditorRenderer::Begin(const RenderView& _view)
{
    JAM_ASSERT(!m_bBeginRender, "EditorRenderer::Begin called without End");

    m_eye  = _view.eye;
    m_view = _view.view;
    m_proj = _view.proj;

    m_numLineVerts   = 0;
    m_numLineIndices = 0;

    m_drawGridFlags = eDrawGridFlags_None;
    m_bBeginRender  = true;

    Graphics::SetFrameBuffer(_view.fbh);
    Graphics::SetViewport(_view.vp);

    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    Graphics::SetBlendState(m_blendState, 0);
}

void EditorRenderer::End()
{
    JAM_ASSERT(m_bBeginRender, "EditorRenderer::End called without Begin");

    FlushLines_();

    if (m_drawGridFlags != eDrawGridFlags_None)
    {
        DrawGrid_();
    }

    m_bBeginRender = false;
}

void EditorRenderer::SetColor(const RGBA _color)
{
    m_currentColor = _color;
}

void EditorRenderer::DrawGrid(const eDrawGridFlags _flags)
{
    m_drawGridFlags = _flags;
}

void EditorRenderer::FlushLines_()
{
    if (m_numLineIndices == 0)
    {
        return;
    }

    CB_EDITOR_RENDERER cb;
    cb.viewProj = (m_view * m_proj).Transpose();
    cb.mtx      = Mat4::Identity.Transpose();
    cb.eye      = m_eye;
    Graphics::UploadBuffer(m_cbh, cb);

    Graphics::UploadBuffer(m_lineVbh, ConstMemView { m_lineVertices, sizeof(LineVertex) * m_numLineVerts });
    Graphics::UploadBuffer(m_lineIbh, ConstMemView { m_lineIndices, sizeof(UInt16) * m_numLineIndices });

    RenderState state = m_lineState;
    state.primitive   = ePrimitive::LineList;
    Graphics::SetRenderState(state);
    Graphics::SetProgram(m_lineProgramHandle);
    Graphics::SetVertexBuffer(m_lineVbh, m_numLineVerts, 0);
    Graphics::SetIndexBuffer(m_lineIbh, m_numLineIndices, 0);
    Graphics::Submit();

    m_numLineVerts   = 0;
    m_numLineIndices = 0;
}

void EditorRenderer::DrawGrid_() const
{
    CB_EDITOR_RENDERER cb;
    cb.viewProj = (m_view * m_proj).Transpose();
    cb.mtx      = Mat4::Identity.Transpose();
    cb.eye      = m_eye;
    cb.flags    = m_drawGridFlags;
    Graphics::UploadBuffer(m_cbh, cb);

    Graphics::SetRenderState(m_gridState);
    Graphics::SetBlendState(m_blendState, 0);
    Graphics::SetProgram(m_gridProgramHandle);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    Graphics::SetVertexBuffer(m_gridVbh);
    Graphics::SetIndexBuffer(m_gridIbh, 6, 0);
    Graphics::Submit();
}

void EditorRenderer::DrawLine(const Vec3 _from, const Vec3 _to, const RGBA _color)
{
    if (m_numLineIndices + 2 >= k_lineLimit * 2)
    {
        FlushLines_();
    }

    UInt16 baseIdx = NarrowCast<UInt16>(m_numLineVerts);

    m_lineVertices[m_numLineVerts++]  = LineVertex { _from, _color };
    m_lineVertices[m_numLineVerts++]  = LineVertex { _to, _color };
    m_lineIndices[m_numLineIndices++] = baseIdx;
    m_lineIndices[m_numLineIndices++] = baseIdx + 1;
}

void EditorRenderer::DrawBoundingBox(const Aabb& _aabbInL, const Mat4& _mtx, const RGBA _color)
{
    Obb obb = Obb::FromAabb(_aabbInL);
    obb.Transform(_mtx);

    const CornerPoints corners = obb.GetCornerPoints();

    // near plane
    DrawLine(corners.nearLeftTop, corners.nearRightTop, _color);
    DrawLine(corners.nearRightTop, corners.nearRightBottom, _color);
    DrawLine(corners.nearRightBottom, corners.nearLeftBottom, _color);
    DrawLine(corners.nearLeftBottom, corners.nearLeftTop, _color);

    // far plane
    DrawLine(corners.farLeftTop, corners.farRightTop, _color);
    DrawLine(corners.farRightTop, corners.farRightBottom, _color);
    DrawLine(corners.farRightBottom, corners.farLeftBottom, _color);
    DrawLine(corners.farLeftBottom, corners.farLeftTop, _color);

    // sides
    DrawLine(corners.nearLeftTop, corners.farLeftTop, _color);
    DrawLine(corners.nearRightTop, corners.farRightTop, _color);
    DrawLine(corners.nearRightBottom, corners.farRightBottom, _color);
    DrawLine(corners.nearLeftBottom, corners.farLeftBottom, _color);
}

void EditorRenderer::DrawFrustum(const Mat4& _viewProjMtx, const RGBA _rgba)
{
    const Frustum      frustum = Frustum::FromMatrix(_viewProjMtx);
    const CornerPoints corners = frustum.GetCornerPoints();

    // near plane
    DrawLine(corners.nearLeftTop, corners.nearRightTop, _rgba);
    DrawLine(corners.nearRightTop, corners.nearRightBottom, _rgba);
    DrawLine(corners.nearRightBottom, corners.nearLeftBottom, _rgba);
    DrawLine(corners.nearLeftBottom, corners.nearLeftTop, _rgba);

    // far plane
    DrawLine(corners.farLeftTop, corners.farRightTop, _rgba);
    DrawLine(corners.farRightTop, corners.farRightBottom, _rgba);
    DrawLine(corners.farRightBottom, corners.farLeftBottom, _rgba);
    DrawLine(corners.farLeftBottom, corners.farLeftTop, _rgba);

    // sides
    DrawLine(corners.nearLeftTop, corners.farLeftTop, _rgba);
    DrawLine(corners.nearRightTop, corners.farRightTop, _rgba);
    DrawLine(corners.nearRightBottom, corners.farRightBottom, _rgba);
    DrawLine(corners.nearLeftBottom, corners.farLeftBottom, _rgba);
}

void EditorRenderer::DrawCircle(const Vec3 _center, const Vec3 _normal, const float _radius, const RGBA _color, const Int32 _segments)
{
    Vec3 u = Vec3::Up;
    if (IsEqualApprox(Dot(_normal, u), 1.f))
    {
        u = Vec3::Right;
    }

    const Vec3 tan   = Normalized(Cross(_normal, u));
    const Vec3 bitan = Normalized(Cross(_normal, tan));

    const float step = k_2pi / static_cast<float>(_segments);
    for (Int32 i = 0; i < _segments; ++i)
    {
        const float a0 = step * static_cast<float>(i);
        const float a1 = step * static_cast<float>((i + 1) % _segments);

        const auto [s0, c0] = SinCosApprox(a0);
        const auto [s1, c1] = SinCosApprox(a1);
        const Vec3 p0       = _center + (tan * c0 + bitan * s0) * _radius;
        const Vec3 p1       = _center + (tan * c1 + bitan * s1) * _radius;
        DrawLine(p0, p1, _color);
    }
}

void EditorRenderer::DrawFilledSphere(const Vec3 _center, const float _radius, const RGBA _color)
{
    FlushLines_();

    m_currentColor = _color;

    Mat4 world = Mat4::CreateScale(_radius) * Mat4::CreateTranslation(_center);
    UploadTransform_(world);

    RenderState state = m_lineState;
    state.primitive   = ePrimitive::TriangleList;
    Graphics::SetRenderState(state);
    Graphics::SetProgram(m_filledProgramHandle);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_sphere.Submit();
}

void EditorRenderer::DrawFilledCube(const Vec3 _center, const Vec3 _size, const RGBA _color)
{
    FlushLines_();

    m_currentColor = _color;

    Mat4 world = Mat4::CreateScale(_size) * Mat4::CreateTranslation(_center);
    UploadTransform_(world);

    RenderState state = m_lineState;
    state.primitive   = ePrimitive::TriangleList;
    Graphics::SetRenderState(state);
    Graphics::SetProgram(m_filledProgramHandle);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_cube.Submit();
}

void EditorRenderer::DrawFilledCylinder(const Vec3 _center, const Vec3 _dir, const float _height, const float _radius, const RGBA _color)
{
    FlushLines_();

    m_currentColor = _color;
    Quat q         = Quat::FromToRotation(Vec3::Up, _dir);
    Mat4 world     = Mat4::CreateScale(Vec3(_radius, _height, _radius)) * Mat4::CreateFromQuaternion(q) * Mat4::CreateTranslation(_center);
    UploadTransform_(world);

    RenderState state = m_lineState;
    state.primitive   = ePrimitive::TriangleList;
    Graphics::SetRenderState(state);
    Graphics::SetProgram(m_filledProgramHandle);
    Graphics::SetConstantBuffer(m_cbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(m_cbh, eShader::PixelShader, 0);
    m_cylinder.Submit();
}

void EditorRenderer::DrawFilledCapsule(const Vec3 _center, const Vec3 _dir, const float _height, const float _radius, const RGBA _color)
{
    float cylinderHeight = Max(0.f, _height - 2.f * _radius);

    // 실린더
    DrawFilledCylinder(_center, _dir, cylinderHeight, _radius, _color);

    // 위쪽 반구
    Vec3 topCenter = _center + _dir * cylinderHeight * 0.5f;
    DrawFilledSphere(topCenter, _radius, _color);

    // 아래쪽 반구
    Vec3 bottomCenter = _center - _dir * cylinderHeight * 0.5f;
    DrawFilledSphere(bottomCenter, _radius, _color);
}

void EditorRenderer::FlushLines()
{
    FlushLines_();
}

void EditorRenderer::UploadTransform_(const Mat4& _world) const
{
    CB_EDITOR_RENDERER cb;
    cb.viewProj = (m_view * m_proj).Transpose();
    cb.mtx      = _world.Transpose();
    cb.eye      = m_eye;
    cb.color    = m_currentColor.ToLinear();
    Graphics::UploadBuffer(m_cbh, cb);
}

}   // namespace jam