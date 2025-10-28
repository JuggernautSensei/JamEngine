#include "pch.h"

#include "DebugRenderer.h"

#include "BaseUtilities.h"
#include "Graphics.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "ShaderCompiler.h"
#include "Shaders/PrecompiledShaders/PrecompiledDebugDrawShader.h"

namespace jam
{

//////////////////////////////////
///          CONSTANT          ///
//////////////////////////////////

constexpr Int32 k_circleSegmentTable[EnumCount<eDetailLevel>()] = {
    16,
    24,
    32,
    40
};

constexpr Int32 k_sphereSubdivisionTable[EnumCount<eDetailLevel>()] = {
    1,   // 80 면체
    2,   // 320 면체
    3,   // 1280 면체
    4    // 5120 면체
};

constexpr Int32 k_sphereCircleTable[EnumCount<eDetailLevel>()] = {
    4,
    6,
    8,
    10
};

constexpr Int32 k_maxBatchLineCount = 512;

///////////////////////////////
///          ENUMS          ///
///////////////////////////////

enum class eDebugRendererMesh : Int8
{
    Box = 0,
    Quad,

    Cone_Low,
    Cone_Mideum,
    Cone_High,
    Cone_Ultra,

    Circle_Low,
    Circle_Mideum,
    Circle_High,
    Circle_Ultra,

    Sphere_Low,
    Sphere_Mideum,
    Sphere_High,
    Sphere_Ultra,

    Cylinder_Low,
    Cylinder_Mideum,
    Cylinder_High,
    Cylinder_Ultra,
};

enum class eDebugRendererProgram : Int8
{
    Line,
    Solid
};

/////////////////////////////////////
///          VERTEX TYPE          ///
/////////////////////////////////////

struct DebugRendererLineVertex
{
    Vec3 position;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        return layout;
    }();
};

struct DebugRendererSolidVertex
{
    Vec3 position;
    Vec3 normal;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        layout.Add(eVertexAttribute::Normal, eAttributeFormat::Float3);
        return layout;
    }();
};

/////////////////////////////////
///          STRUCTS          ///
/////////////////////////////////

/// 디버그 드로우 용 상수 버퍼
struct alignas(16) CB_DEBUGRENDERER
{
    Mat4 projView;
    Mat4 world;
    Mat4 worldInvTrans;
    Vec4 color;
    //
    Vec3                   eyePos;
    JAM_MAYBE_UNUSED float pad1;
};

/// 디버그 드로우의 렌더링 상태
struct DebugRendererState
{
    RGBA         color       = RGBA { 0xFFFFFFFF };
    GfxState     state       = GfxState::k_depthTest;
    eDetailLevel detailLevel = eDetailLevel::Medium;
    bool         bWireframe  = false;
};

/////////////////////////////////
///          CONTEXT          ///
/////////////////////////////////

/// 디버그 드로우 컨텍스트
struct DrawRendererContext
{
    /// 드로우 콜
    void DrawMesh(const Mesh& _mesh, bool _bWireframe);
    void DrawLineBatch();

    /// 저수준 드로우 콜
    void MoveTo(float _x, float _y, float _z);
    void LineTo(float _x, float _y, float _z);

    /// 변환
    void SetShapeTransform(const Mat4& _mat);

    // 변환
    std::vector<Mat4> transformStack;
    Mat4              curTransform = Mat4::Identity;

    // 렌더링 상태
    std::vector<DebugRendererState> stateStack;
    DebugRendererState              curState = {};

    // 사전 정의된 메시
    Mesh debugDrawMeshes[EnumCount<eDebugRendererMesh>()] = {};

    // 라인 드로우 배치
    DebugRendererLineVertex lineVerts[k_maxBatchLineCount * 2] = {};
    Int32                   lineVertsCount                     = 0;

    UInt16 lineIndices[k_maxBatchLineCount * 2] = {};
    Int32  lineIndicesCount                     = 0;

    VertexBufferHandle lineVbh     = k_invalidHandle;
    IndexBufferHandle  lineIbh     = k_invalidHandle;
    Vec3               cursorPos   = Vec3::Zero;
    bool               bCursorDown = false;

    ProgramHandle        programHandles[EnumCount<eDebugRendererProgram>()] = {};
    ConstantBufferHandle debugDrawCbh;
    CB_DEBUGRENDERER     debugDrawCpu;

} g_debugRendererContext;

void DrawRendererContext::MoveTo(const float _x, const float _y, const float _z)
{
    bCursorDown = false;
    cursorPos   = Vec3 { _x, _y, _z };
}

void DrawRendererContext::LineTo(const float _x, const float _y, const float _z)
{
    Int32 lineCount = lineIndicesCount / 2;

    // 버퍼가 가득 차있으면 flush
    if (lineCount == k_maxBatchLineCount)
    {
        DrawLineBatch();
        lineVertsCount   = 0;
        lineIndicesCount = 0;
    }

    // 새로운 라인. 시작점 추가.
    if (bCursorDown == false)
    {
        lineVerts[lineVertsCount++] = DebugRendererLineVertex { cursorPos };
    }

    // 라인 그리기 활성화
    bCursorDown = true;

    // 끝점 추가
    Vec3 to                     = { _x, _y, _z };
    lineVerts[lineVertsCount++] = DebugRendererLineVertex { to };

    // 인덱스 추가
    UInt16 index                    = NarrowCast<UInt16>(lineVertsCount - 2);   // 시작점 인덱스
    lineIndices[lineIndicesCount++] = index;
    lineIndices[lineIndicesCount++] = index + 1;

    // 커서 이동
    cursorPos = to;
}

void DrawRendererContext::DrawMesh(const Mesh& _mesh, const bool _bWireframe)
{
    // 상태 업데이트
    GfxState state  = curState.state;
    state.primitive = ePrimitive::TriangleList;
    state.cullMode  = eCullMode::Back;

    // 와이어 프레임의 경우
    if (_bWireframe)
        state.bWireframe = true;

    // 상태 설정
    Graphics::SetGfxState(state);

    // 상수 버퍼 업데이트
    RGBA color                 = curState.color;
    debugDrawCpu.color         = Vec4(color.r, color.g, color.b, color.a) / 255.f;
    debugDrawCpu.world         = curTransform.Transpose();
    debugDrawCpu.worldInvTrans = curTransform.Invert();
    Graphics::UploadBuffer(debugDrawCbh, MemoryView { debugDrawCpu });

    // 상수 버퍼 바인딩
    Graphics::SetConstantBuffer(debugDrawCbh, eShader::VertexShader, 0);
    Graphics::SetConstantBuffer(debugDrawCbh, eShader::PixelShader, 0);

    // 프로그램 바인딩
    Graphics::SetProgram(programHandles[EnumToInt(eDebugRendererProgram::Solid)]);

    // 메시 렌더링
    Graphics::SetVertexBuffer(_mesh.GetVertexBuffer());
    Graphics::SetIndexBuffer(_mesh.GetIndexBuffer());
    Graphics::Submit();
}

void DrawRendererContext::DrawLineBatch()
{
    Int32 lineCount = lineIndicesCount / 2;
    if (lineCount > 0)
    {
        GfxState state  = curState.state;
        state.primitive = ePrimitive::LineList;
        state.cullMode  = eCullMode::None;

        // 상태 업데이트
        Graphics::SetGfxState(state);

        // 상수 버퍼 업로드
        RGBA color                 = curState.color;
        debugDrawCpu.color         = Vec4(color.r, color.g, color.b, color.a) / 255.f;
        debugDrawCpu.world         = curTransform.Transpose();
        debugDrawCpu.worldInvTrans = curTransform.Invert();
        Graphics::UploadBuffer(debugDrawCbh, MemoryView { debugDrawCpu });

        // 상수 버퍼 바인딩
        Graphics::SetConstantBuffer(debugDrawCbh, eShader::VertexShader, 0);
        Graphics::SetConstantBuffer(debugDrawCbh, eShader::PixelShader, 0);

        // 버텍스 버퍼 업로드
        Graphics::UploadBuffer(lineVbh, MemoryView { lineVerts, sizeof(DebugRendererLineVertex) * lineVertsCount });
        Graphics::UploadBuffer(lineIbh, MemoryView { lineIndices, sizeof(UInt16) * lineIndicesCount });

        // 버텍스 버퍼 바인딩
        Graphics::SetVertexBuffer(lineVbh, lineVertsCount, 0);
        Graphics::SetIndexBuffer(lineIbh, lineIndicesCount, 0);

        // 프로그램 바인딩
        Graphics::SetProgram(programHandles[EnumToInt(eDebugRendererProgram::Line)]);

        // 라인 렌더링
        Graphics::Submit();

        // 커맨드 버퍼 클리어
        lineVertsCount   = 0;
        lineIndicesCount = 0;
    }
}

void DrawRendererContext::SetShapeTransform(const Mat4& _mat)
{
    // 기존 SetTransform 은
    // 부모 행렬 * 자식 행렬 * ... 순서 였음
    // 즉, 부모 변환 -> 자식 변환 -> .. 순서로 적용됨
    // 쉐이프 변환이 필요한 이유는 단위 쉐이프에 스케일, 회전, 이동을 적용하기 위함
    // 최종적으로 버텍스는
    // 쉐이프행렬 -> 부모 -> 자식 -> ... -> 뷰프로젝션 순서로 적용되어야 함
    const Mat4& parentMat = transformStack.back();
    curTransform          = _mat * parentMat;
}
}   // namespace jam

namespace jam
{

void DebugRenderer::Init()
{
    g_debugRendererContext.transformStack.clear();
    g_debugRendererContext.curTransform = Mat4::Identity;

    g_debugRendererContext.stateStack.clear();
    g_debugRendererContext.curState = {};

    // 라인 드로우 버텍스 버퍼
    g_debugRendererContext.lineVertsCount   = 0;
    g_debugRendererContext.lineIndicesCount = 0;

    g_debugRendererContext.lineVbh = Graphics::CreateDynamicVertexBuffer(k_maxBatchLineCount * 2, DebugRendererLineVertex::s_layout);
    g_debugRendererContext.lineIbh = Graphics::CreateDynamicIndexBuffer(k_maxBatchLineCount * 2, eIndexFormat::U16);

    g_debugRendererContext.cursorPos   = Vec3::Zero;
    g_debugRendererContext.bCursorDown = false;

    // 라인 쉐이더
    {
        ShaderHandle vsHandle = Graphics::CreateShader(MemoryView {
            precompiled_shader::DebugDrawLineShader_VS_Bytecode, precompiled_shader::DebugDrawLineShader_VS_Size });

        ShaderHandle psHandle = Graphics::CreateShader(MemoryView {
            precompiled_shader::DebugDrawLineShader_PS_Bytecode, precompiled_shader::DebugDrawLineShader_PS_Size });

        g_debugRendererContext.programHandles[EnumToInt(eDebugRendererProgram::Line)] = Graphics::CreateProgram(vsHandle, psHandle, true);
    }

    // 솔리드 쉐이더
    {
        ShaderHandle vsHandle = Graphics::CreateShader(MemoryView {
            precompiled_shader::DebugDrawSolidShader_VS_Bytecode, precompiled_shader::DebugDrawSolidShader_VS_Size });

        ShaderHandle psHandle = Graphics::CreateShader(MemoryView {
            precompiled_shader::DebugDrawSolidShader_PS_Bytecode, precompiled_shader::DebugDrawSolidShader_PS_Size });

        g_debugRendererContext.programHandles[EnumToInt(eDebugRendererProgram::Solid)] = Graphics::CreateProgram(vsHandle, psHandle, true);
    }

    // 상수 버퍼
    g_debugRendererContext.debugDrawCbh = Graphics::CreateConstantBuffer(sizeof(CB_DEBUGRENDERER));

    // 기본 메쉬 생성
    // 박스
    {
        Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Box)];
        mesh       = CreateBoxMesh(Vec3 { 1.f, 1.f, 1.f }, DebugRendererSolidVertex::s_layout);
    }

    // 쿼드
    {
        Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Quad)];
        mesh       = CreatePlaneMesh(Vec2 { 1.f, 1.f }, DebugRendererSolidVertex::s_layout);
    }

    // 콘
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Cone_Low) + i];
            mesh       = CreateCone(1.f, 1.f, k_circleSegmentTable[i], DebugRendererSolidVertex::s_layout);
        }
    }

    // 원
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Circle_Low) + i];
            mesh       = CreateCircle(1.f, k_circleSegmentTable[i], DebugRendererSolidVertex::s_layout);
        }
    }

    // 구
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Sphere_Low) + i];
            mesh       = CreateSphereMesh(1.f, k_sphereSubdivisionTable[i], DebugRendererSolidVertex::s_layout);
        }
    }

    // 실린더
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Cylinder_Low) + i];
            mesh       = CreateCylinder(1.f, 1.f, k_circleSegmentTable[i], DebugRendererSolidVertex::s_layout);
        }
    }
}

void DebugRenderer::Shutdown()
{
    g_debugRendererContext.transformStack.clear();
    g_debugRendererContext.curTransform = Mat4::Identity;

    g_debugRendererContext.stateStack.clear();
    g_debugRendererContext.curState = {};

    // 메시 정리
    for (Mesh& mesh: g_debugRendererContext.debugDrawMeshes)
    {
        mesh.Unload();
    }

    // 라인 드로우 정리
    g_debugRendererContext.lineVertsCount   = 0;
    g_debugRendererContext.lineIndicesCount = 0;

    JAM_DESTROY_VERTEX_BUFFER(g_debugRendererContext.lineVbh);
    JAM_DESTROY_INDEX_BUFFER(g_debugRendererContext.lineIbh);

    // 프로그램 정리
    for (ProgramHandle handle: g_debugRendererContext.programHandles)
    {
        JAM_DESTROY_PROGRAM(handle);
    }

    // 상수 버퍼 정리
    JAM_DESTROY_CONSTANT_BUFFER(g_debugRendererContext.debugDrawCbh);
}

void DebugRenderer::Begin()
{
    g_debugRendererContext.transformStack.clear();
    g_debugRendererContext.transformStack.push_back(g_debugRendererContext.curTransform);
    g_debugRendererContext.curTransform = Mat4::Identity;

    g_debugRendererContext.stateStack.clear();
    g_debugRendererContext.stateStack.push_back(g_debugRendererContext.curState);
    g_debugRendererContext.curState = {};

    g_debugRendererContext.lineVertsCount   = 0;
    g_debugRendererContext.lineIndicesCount = 0;

    g_debugRendererContext.cursorPos   = Vec3::Zero;
    g_debugRendererContext.bCursorDown = false;

    g_debugRendererContext.curState = {};
}

void DebugRenderer::End()
{
    g_debugRendererContext.DrawLineBatch();
}

void DebugRenderer::PushTransform()
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.transformStack.push_back(g_debugRendererContext.curTransform);
    g_debugRendererContext.curTransform = g_debugRendererContext.transformStack.back();
}

void DebugRenderer::PopTransform()
{
    g_debugRendererContext.DrawLineBatch();
    JAM_ASSERT(g_debugRendererContext.transformStack.size() > 1, "DebugDraw::PopTransform() called with empty stack");
    g_debugRendererContext.curTransform = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.transformStack.pop_back();
}

void DebugRenderer::SetCamera(const Vec3& _eyePos, const Mat4& _viewProj)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.debugDrawCpu.eyePos   = _eyePos;
    g_debugRendererContext.debugDrawCpu.projView = _viewProj.Transpose();
}

void DebugRenderer::SetTransform(const Mat4& _mat)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * _mat;
}

void DebugRenderer::SetTranslate(const Vec3& _translate)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * Mat4::CreateTranslation(_translate);
}

void DebugRenderer::SetRotate(const Quat& _rotate)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * Mat4::CreateFromQuaternion(_rotate);
}

void DebugRenderer::SetRotate(const Vec3& _axis, const float _angleRad)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * Mat4::CreateFromAxisAngle(_axis, _angleRad);
}

void DebugRenderer::SetRotate(const Vec3& _yawPitchRollRad)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * Mat4::CreateFromYawPitchRoll(_yawPitchRollRad);
}

void DebugRenderer::SetScale(const Vec3& _scale)
{
    g_debugRendererContext.DrawLineBatch();
    const Mat4& parentMat               = g_debugRendererContext.transformStack.back();
    g_debugRendererContext.curTransform = parentMat * Mat4::CreateScale(_scale);
}

void DebugRenderer::MultiplyTransform(const Mat4& _mat)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= _mat;
}

void DebugRenderer::MultiplyTranslate(const Vec3& _translate)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= Mat4::CreateTranslation(_translate);
}

void DebugRenderer::MultiplyRotate(const Quat& _rotate)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= Mat4::CreateFromQuaternion(_rotate);
}

void DebugRenderer::MultiplyRotate(const Vec3& _axis, const float _angleRad)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= Mat4::CreateFromAxisAngle(_axis, _angleRad);
}

void DebugRenderer::MultiplyRotate(const Vec3& _yawPitchRollRad)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= Mat4::CreateFromYawPitchRoll(_yawPitchRollRad);
}

void DebugRenderer::MultiplyScale(const Vec3& _scale)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curTransform *= Mat4::CreateScale(_scale);
}

void DebugRenderer::PushState()
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.stateStack.push_back(g_debugRendererContext.curState);
    g_debugRendererContext.curState = g_debugRendererContext.stateStack.back();
}

void DebugRenderer::PopState()
{
    g_debugRendererContext.DrawLineBatch();
    JAM_ASSERT(g_debugRendererContext.stateStack.size() > 1, "DebugDraw::PopState() called with empty stack");
    g_debugRendererContext.curState = g_debugRendererContext.stateStack.back();
    g_debugRendererContext.stateStack.pop_back();
}

void DebugRenderer::SetColor(const RGBA _rgba)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curState.color = _rgba;
}

void DebugRenderer::SetDepthState(const bool _bDepthTest, const bool _bDepthWrite)
{
    g_debugRendererContext.DrawLineBatch();
    GfxState& state     = g_debugRendererContext.curState.state;
    state.depthTestFunc = _bDepthTest ? eCompareFunc::Less : eCompareFunc::None;
    state.bDepthWrite   = _bDepthWrite;
}

void DebugRenderer::SetWireframe(const bool _bWireframe)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curState.bWireframe = _bWireframe;
}

void DebugRenderer::SetDetailLevel(const eDetailLevel _level)
{
    g_debugRendererContext.DrawLineBatch();
    g_debugRendererContext.curState.detailLevel = _level;
}

void DebugRenderer::MoveTo(const Vec3& _pos)
{
    g_debugRendererContext.MoveTo(_pos.x, _pos.y, _pos.z);
}

void DebugRenderer::LineTo(const Vec3& _pos)
{
    g_debugRendererContext.LineTo(_pos.x, _pos.y, _pos.z);
}

void DebugRenderer::DrawLine(const Vec3& _from, const Vec3& _to)
{
    g_debugRendererContext.MoveTo(_from.x, _from.y, _from.z);
    g_debugRendererContext.LineTo(_to.x, _to.y, _to.z);
}

void DebugRenderer::DrawArc(const Vec3& _center, const float _radius, const Vec3& _normal, const float _startAngleRad, const float _endAngleRad)
{
    JAM_ASSERT(_radius > 0.f, "Arc radius must be greater than zero");
    JAM_ASSERT(_startAngleRad < _endAngleRad, "Arc start angle must be less than end angle");

    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _normal, Vec3(_radius)));

    Int32 segment   = k_circleSegmentTable[EnumToInt(g_debugRendererContext.curState.detailLevel)];
    float stepRad   = k_2pi / static_cast<float>(segment);
    Int32 stepCount = static_cast<Int32>(Ceil(Absolute(_endAngleRad - _startAngleRad) / stepRad));

    auto [startSin, startCos] = ComputeSinCos(_startAngleRad);
    g_debugRendererContext.MoveTo(startCos, 0.f, startSin);

    for (Int32 i = 1; i <= stepCount; ++i)
    {
        float angle     = Min(_startAngleRad + stepRad * static_cast<float>(i), _endAngleRad);
        auto [sin, cos] = ComputeSinCos(angle);
        g_debugRendererContext.LineTo(cos, 0.f, sin);
    }

    PopTransform();
}

void DebugRenderer::DrawAABB(const Vec3& _min, const Vec3& _max)
{
    Vec3 size   = _max - _min;
    Vec3 center = 0.5f * (_min + _max);
    DrawOBB(center, size, Vec3::UnitY);
}

void DebugRenderer::DrawOBB(const Vec3& _center, const Vec3& _size, const Vec3& _normal)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _normal, _size));

    if (g_debugRendererContext.curState.bWireframe)
    {
        // 앞 사각형
        g_debugRendererContext.MoveTo(-0.5f, -0.5f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, -0.5f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, +0.5f, -0.5f);
        g_debugRendererContext.LineTo(-0.5f, +0.5f, -0.5f);
        g_debugRendererContext.LineTo(-0.5f, -0.5f, -0.5f);

        // 뒷 사각형
        g_debugRendererContext.MoveTo(-0.5f, -0.5f, +0.5f);
        g_debugRendererContext.LineTo(+0.5f, -0.5f, +0.5f);
        g_debugRendererContext.LineTo(+0.5f, +0.5f, +0.5f);
        g_debugRendererContext.LineTo(-0.5f, +0.5f, +0.5f);
        g_debugRendererContext.LineTo(-0.5f, -0.5f, +0.5f);

        // 옆 줄
        g_debugRendererContext.MoveTo(-0.5f, -0.5f, -0.5f);
        g_debugRendererContext.LineTo(-0.5f, -0.5f, +0.5f);
        g_debugRendererContext.MoveTo(+0.5f, -0.5f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, -0.5f, +0.5f);
        g_debugRendererContext.MoveTo(+0.5f, +0.5f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, +0.5f, +0.5f);
        g_debugRendererContext.MoveTo(-0.5f, +0.5f, -0.5f);
        g_debugRendererContext.LineTo(-0.5f, +0.5f, +0.5f);
    }
    else
    {
        const Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Box)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DebugRenderer::DrawSphere(const Vec3& _center, const float _radius)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, Vec3::UnitY, Vec3(_radius)));

    if (g_debugRendererContext.curState.bWireframe)
    {
        Int32 circleCount = k_sphereCircleTable[EnumToInt(g_debugRendererContext.curState.detailLevel)];

        // 경도
        float stepRad = k_2pi / static_cast<float>(circleCount);
        for (Int32 i = 0; i < circleCount; ++i)
        {
            auto [sin, cos] = ComputeSinCos(i * stepRad);
            Vec3 normal     = { cos, 0.f, sin };
            DrawArc(Vec3::Zero, 1.f, normal, 0.f, k_2pi);
        }

        // 위도
        float yStep = 2.f / static_cast<float>(circleCount);
        for (Int32 i = 1; i < circleCount; ++i)
        {
            float y = -1.f + yStep * static_cast<float>(i);
            float r = Sqrt(1.f - y * y);
            DrawArc(Vec3::Zero, r, Vec3::UnitY, 0.f, k_2pi);
        }
    }
    else
    {
        eDebugRendererMesh meshType = static_cast<eDebugRendererMesh>(EnumToInt(eDebugRendererMesh::Sphere_Low) + EnumToInt(g_debugRendererContext.curState.detailLevel));
        const Mesh&        mesh     = g_debugRendererContext.debugDrawMeshes[EnumToInt(meshType)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }
    PopTransform();
}

void DebugRenderer::DrawQuad(const Vec3& _center, const Vec2& _size, const Vec3& _normal)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _normal, Vec3(_size.x, 1.f, _size.y)));

    if (g_debugRendererContext.curState.bWireframe)
    {
        g_debugRendererContext.MoveTo(-0.5f, 0.f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, 0.f, -0.5f);
        g_debugRendererContext.LineTo(+0.5f, 0.f, +0.5f);
        g_debugRendererContext.LineTo(-0.5f, 0.f, +0.5f);
        g_debugRendererContext.LineTo(-0.5f, 0.f, -0.5f);
    }
    else
    {
        const Mesh& mesh = g_debugRendererContext.debugDrawMeshes[EnumToInt(eDebugRendererMesh::Quad)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DebugRenderer::DrawGrid(const Vec3& _center, const Vec2& _size, const Vec3& _normal, const float _step)
{
    JAM_ASSERT(_step > 0.f, "Grid step must be greater than zero");

    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _normal, Vec3(1.f)));

    const int halfLineCountX = static_cast<int>(_size.x / _step * 0.5f);
    const int halfLineCountY = static_cast<int>(_size.y / _step * 0.5f);
    for (int i = -halfLineCountX; i <= halfLineCountX; ++i)
    {
        const float x = static_cast<float>(i) * _step;
        g_debugRendererContext.MoveTo(x, 0.f, static_cast<float>(-halfLineCountY) * _step);
        g_debugRendererContext.LineTo(x, 0.f, static_cast<float>(+halfLineCountY) * _step);
    }

    for (int i = -halfLineCountY; i <= halfLineCountY; ++i)
    {
        const float z = static_cast<float>(i) * _step;
        g_debugRendererContext.MoveTo(static_cast<float>(-halfLineCountX) * _step, 0.f, z);
        g_debugRendererContext.LineTo(static_cast<float>(+halfLineCountX) * _step, 0.f, z);
    }

    PopTransform();
}

void DebugRenderer::DrawCircle(const Vec3& _center, const float _radius, const Vec3& _normal)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _normal, Vec3(_radius)));

    if (g_debugRendererContext.curState.bWireframe)
    {
        DrawArc(Vec3(0.f, 0.f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);
    }
    else
    {
        eDebugRendererMesh meshType = static_cast<eDebugRendererMesh>(EnumToInt(eDebugRendererMesh::Circle_Low) + EnumToInt(g_debugRendererContext.curState.detailLevel));
        const Mesh&        mesh     = g_debugRendererContext.debugDrawMeshes[EnumToInt(meshType)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }
}

void DebugRenderer::DrawCone(const Vec3& _center, const float _height, const float _radius, const Vec3& _dir)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _dir, Vec3(_radius, _height, _radius)));

    if (g_debugRendererContext.curState.bWireframe)
    {
        // 밑면 원
        DrawArc(Vec3(0.f, -0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);

        // 옆면 선
        Int32 segment = k_circleSegmentTable[EnumToInt(g_debugRendererContext.curState.detailLevel)];
        float stepRad = k_2pi / static_cast<float>(segment);
        for (Int32 i = 0; i < segment; ++i)
        {
            auto [sin, cos] = ComputeSinCos(i * stepRad);
            g_debugRendererContext.MoveTo(0.f, +0.5f, 0.f);
            g_debugRendererContext.LineTo(cos, -0.5f, sin);
        }
    }
    else
    {
        eDebugRendererMesh meshType = static_cast<eDebugRendererMesh>(EnumToInt(eDebugRendererMesh::Cone_Low) + EnumToInt(g_debugRendererContext.curState.detailLevel));
        const Mesh&        mesh     = g_debugRendererContext.debugDrawMeshes[EnumToInt(meshType)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DebugRenderer::DrawFrustum(const Frustum& _frustum)
{
    // 절두체
    FrustumCorners corners = _frustum.GetCorners();

    // 근평면
    g_debugRendererContext.MoveTo(corners.nearLeftTop.x, corners.nearLeftTop.y, corners.nearLeftTop.z);
    g_debugRendererContext.LineTo(corners.nearRightTop.x, corners.nearRightTop.y, corners.nearRightTop.z);
    g_debugRendererContext.LineTo(corners.nearRightBottom.x, corners.nearRightBottom.y, corners.nearRightBottom.z);
    g_debugRendererContext.LineTo(corners.nearLeftBottom.x, corners.nearLeftBottom.y, corners.nearLeftBottom.z);
    g_debugRendererContext.LineTo(corners.nearLeftTop.x, corners.nearLeftTop.y, corners.nearLeftTop.z);

    // 원평면
    g_debugRendererContext.MoveTo(corners.farLeftTop.x, corners.farLeftTop.y, corners.farLeftTop.z);
    g_debugRendererContext.LineTo(corners.farRightTop.x, corners.farRightTop.y, corners.farRightTop.z);
    g_debugRendererContext.LineTo(corners.farRightBottom.x, corners.farRightBottom.y, corners.farRightBottom.z);
    g_debugRendererContext.LineTo(corners.farLeftBottom.x, corners.farLeftBottom.y, corners.farLeftBottom.z);
    g_debugRendererContext.LineTo(corners.farLeftTop.x, corners.farLeftTop.y, corners.farLeftTop.z);

    // 옆면 선 4개
    g_debugRendererContext.MoveTo(corners.nearLeftTop.x, corners.nearLeftTop.y, corners.nearLeftTop.z);
    g_debugRendererContext.LineTo(corners.farLeftTop.x, corners.farLeftTop.y, corners.farLeftTop.z);
    g_debugRendererContext.MoveTo(corners.nearRightTop.x, corners.nearRightTop.y, corners.nearRightTop.z);
    g_debugRendererContext.LineTo(corners.farRightTop.x, corners.farRightTop.y, corners.farRightTop.z);
    g_debugRendererContext.MoveTo(corners.nearRightBottom.x, corners.nearRightBottom.y, corners.nearRightBottom.z);
    g_debugRendererContext.LineTo(corners.farRightBottom.x, corners.farRightBottom.y, corners.farRightBottom.z);
    g_debugRendererContext.MoveTo(corners.nearLeftBottom.x, corners.nearLeftBottom.y, corners.nearLeftBottom.z);
    g_debugRendererContext.LineTo(corners.farLeftBottom.x, corners.farLeftBottom.y, corners.farLeftBottom.z);
}

void DebugRenderer::DrawCylinder(const Vec3& _center, const float _height, const float _radius, const Vec3& _dir)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _dir, Vec3(_radius, _height, _radius)));

    if (g_debugRendererContext.curState.bWireframe)
    {
        // 위, 아래 원
        DrawArc(Vec3(0.f, -0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);
        DrawArc(Vec3(0.f, +0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);

        // 옆면 선
        Int32 segment = k_circleSegmentTable[EnumToInt(g_debugRendererContext.curState.detailLevel)];
        float stepRad = k_2pi / static_cast<float>(segment);
        for (Int32 i = 0; i < segment; ++i)
        {
            auto [sin, cos] = ComputeSinCos(i * stepRad);
            g_debugRendererContext.MoveTo(cos, -0.5f, sin);
            g_debugRendererContext.LineTo(cos, +0.5f, sin);
        }
    }
    else
    {
        eDebugRendererMesh meshType = static_cast<eDebugRendererMesh>(EnumToInt(eDebugRendererMesh::Cylinder_Low) + EnumToInt(g_debugRendererContext.curState.detailLevel));
        const Mesh&        mesh     = g_debugRendererContext.debugDrawMeshes[EnumToInt(meshType)];
        g_debugRendererContext.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DebugRenderer::DrawCapsule(const Vec3& _center, const Vec3& _dir, const float _height, const float _radius)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_center, _dir, Vec3(_radius, _height, _radius)));

    // 원통
    float cylinderHeight = Max(0.f, _height - 2.f * _radius);
    DrawCylinder(Vec3::Zero, cylinderHeight, 1.f, Vec3::UnitY);

    if (g_debugRendererContext.curState.bWireframe)
    {
        // 반구
        Int32 circleCount = k_sphereCircleTable[EnumToInt(g_debugRendererContext.curState.detailLevel)];
        float yStep       = 1.f / static_cast<float>(circleCount);
        for (Int32 i = 0; i < circleCount; ++i)
        {
            float y = yStep * static_cast<float>(i);
            float r = Sqrt(1.f - y * y);
            DrawArc(Vec3(0.f, +0.5f * cylinderHeight, 0.f), r, Vec3::UnitY, 0.f, k_2pi);
            DrawArc(Vec3(0.f, -0.5f * cylinderHeight, 0.f), r, Vec3::UnitY, 0.f, k_2pi);
        }
    }
    else
    {
        // 구 메시를 2번 그림
        DrawSphere(Vec3(0.f, +0.5f * cylinderHeight, 0.f), 1.f);
        DrawSphere(Vec3(0.f, -0.5f * cylinderHeight, 0.f), 1.f);
    }

    PopTransform();
}

void DebugRenderer::DrawMesh(const Mesh& _mesh, const Vec3& _position, const Vec3& _forward, const Vec3& _scale)
{
    PushTransform();
    g_debugRendererContext.SetShapeTransform(CreateWorldLookTo(_position, _forward, _scale));
    g_debugRendererContext.DrawMesh(_mesh, g_debugRendererContext.curState.bWireframe);
    PopTransform();
}

}   // namespace jam