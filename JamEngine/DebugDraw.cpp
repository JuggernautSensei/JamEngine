#include "pch.h"

#include "DebugDraw.h"

#include "BaseUtilities.h"
#include "Gfx.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "ShaderCompiler.h"
#include "Shaders/PrecompiledShaders/PrecompiledDebugDrawShader.h"
#include "fixed_vector.h"
using namespace jam;

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

enum class eDebugDrawMesh : Int8
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

enum class eDebugDrawProgram : Int8
{
    Line,
    Solid
};

/////////////////////////////////////
///          VERTEX TYPE          ///
/////////////////////////////////////

struct DebugDrawLineVertex
{
    Vec3 position;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        return layout;
    }();
};

struct DebugDrawSolidVertex
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
struct alignas(16) CB_DEBUGDRAW
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
struct DebugDrawState
{
    RGBA         color       = 0xFFFFFFFF;
    GfxState     state       = GfxState::k_depthTest;
    eDetailLevel detailLevel = eDetailLevel::Medium;
    bool         bWireframe  = false;
};

/////////////////////////////////
///          CONTEXT          ///
/////////////////////////////////

/// 디버그 드로우 컨텍스트
struct DebugDrawContext
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
    std::vector<DebugDrawState> stateStack;
    DebugDrawState              curState = {};

    // 사전 정의된 메시
    Mesh debugDrawMeshes[EnumCount<eDebugDrawMesh>()] = {};

    // 라인 드로우 배치
    fixed_vector<DebugDrawLineVertex, k_maxBatchLineCount * 2> lineVertices = {};
    fixed_vector<Int16, k_maxBatchLineCount * 2>               lineIndices  = {};

    VertexBufferHandle lineVertexBufferHandle = k_invalidHandle;
    IndexBufferHandle  lineIndexBufferHandle  = k_invalidHandle;
    Vec3               cursor                 = Vec3::Zero;
    bool               bCursorDown            = false;

    ProgramHandle        programHandles[EnumCount<eDebugDrawProgram>()] = {};
    ConstantBufferHandle cbDebugDrawHandle;
    CB_DEBUGDRAW         cbDebugDrawCPU;
};

static DebugDrawContext g_ddCtx;

void DebugDrawContext::MoveTo(const float _x, const float _y, const float _z)
{
    bCursorDown = false;
    cursor      = Vec3 { _x, _y, _z };
}

void DebugDrawContext::LineTo(const float _x, const float _y, const float _z)
{
    Int32 lineCount = static_cast<Int32>(lineIndices.size() / 2);

    // 버퍼가 가득 차있으면 flush
    if (lineCount == k_maxBatchLineCount)
    {
        DrawLineBatch();
        lineVertices.clear();
        lineIndices.clear();
    }

    // 새로운 라인. 시작점 추가.
    if (bCursorDown == false)
    {
        lineVertices.push_back(DebugDrawLineVertex { cursor });
    }

    // 라인 그리기 활성화
    bCursorDown = true;

    // 끝점 추가
    Vec3 to = { _x, _y, _z };
    lineVertices.push_back(DebugDrawLineVertex { to });

    // 인덱스 추가
    UInt16 startIndex = static_cast<UInt16>(lineVertices.size() - 2);   // 시작점 인덱스
    lineIndices.push_back(startIndex);
    lineIndices.push_back(startIndex + 1);

    // 커서 이동
    cursor = to;
}

void DebugDrawContext::DrawMesh(const Mesh& _mesh, const bool _bWireframe)
{
    // 상태 업데이트
    GfxState state  = curState.state;
    state.primitive = ePrimitive::TriangleList;
    state.cullMode  = eCullMode::Back;

    // 와이어 프레임의 경우
    if (_bWireframe)
        state.bWireframe = true;

    gfx::SetGfxState(state);

    // 상수 버퍼 업데이트
    RGBA color                       = curState.color;
    cbDebugDrawCPU.color         = Vec4(color.r, color.g, color.b, color.a) / 255.f;
    cbDebugDrawCPU.world         = curTransform.Transpose();
    cbDebugDrawCPU.worldInvTrans = curTransform.Invert();
    gfx::UploadBuffer(cbDebugDrawHandle, MemoryView { cbDebugDrawCPU });

    // 상수 버퍼 바인딩
    gfx::SetConstantBuffer(cbDebugDrawHandle, eShader::VertexShader, 0);
    gfx::SetConstantBuffer(cbDebugDrawHandle, eShader::PixelShader, 0);

    // 프로그램 바인딩
    gfx::SetProgram(programHandles[EnumToInt(eDebugDrawProgram::Solid)]);

    // 메시 렌더링
    gfx::SetVertexBuffer(_mesh.GetVertexBuffer());
    gfx::SetIndexBuffer(_mesh.GetIndexBuffer());
    gfx::Submit();
}

void DebugDrawContext::DrawLineBatch()
{
    Int32 lineCount = static_cast<Int32>(lineIndices.size() / 2);
    if (lineCount > 0)
    {
        GfxState state  = curState.state;
        state.primitive = ePrimitive::LineList;
        state.cullMode  = eCullMode::None;

        // 상태 업데이트
        gfx::SetGfxState(state);

        // 상수 버퍼 업로드
        RGBA color                       = curState.color;
        cbDebugDrawCPU.color         = Vec4(color.r, color.g, color.b, color.a) / 255.f;
        cbDebugDrawCPU.world         = curTransform.Transpose();
        cbDebugDrawCPU.worldInvTrans = curTransform.Invert();
        gfx::UploadBuffer(cbDebugDrawHandle, MemoryView { cbDebugDrawCPU });

        // 상수 버퍼 바인딩
        gfx::SetConstantBuffer(cbDebugDrawHandle, eShader::VertexShader, 0);
        gfx::SetConstantBuffer(cbDebugDrawHandle, eShader::PixelShader, 0);

        // 버텍스 버퍼 업로드
        gfx::UploadBuffer(lineVertexBufferHandle, MemoryView { lineVertices });
        gfx::UploadBuffer(lineIndexBufferHandle, MemoryView { lineIndices });

        // 버텍스 버퍼 바인딩
        Int32 vertCount  = NarrowCast<Int32>(lineVertices.size());
        Int32 indexCount = NarrowCast<Int32>(lineIndices.size());
        gfx::SetVertexBuffer(lineVertexBufferHandle, vertCount, 0);
        gfx::SetIndexBuffer(lineIndexBufferHandle, indexCount, 0);

        // 프로그램 바인딩
        gfx::SetProgram(programHandles[EnumToInt(eDebugDrawProgram::Line)]);

        // 라인 렌더링
        gfx::Submit();

        // 커맨드 버퍼 클리어
        lineVertices.clear();
        lineIndices.clear();
    }
}

void DebugDrawContext::SetShapeTransform(const Mat4& _mat)
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

////////////////////////////////////////////////////////////

namespace jam::debug_draw
{

void Init()
{
    g_ddCtx.transformStack.clear();
    g_ddCtx.curTransform = Mat4::Identity;

    g_ddCtx.stateStack.clear();
    g_ddCtx.curState = {};

    // 라인 드로우 버텍스 버퍼
    g_ddCtx.lineVertices.clear();
    g_ddCtx.lineIndices.clear();

    g_ddCtx.lineVertexBufferHandle = gfx::CreateDynamicVertexBuffer(k_maxBatchLineCount * 2, DebugDrawLineVertex::s_layout);
    g_ddCtx.lineIndexBufferHandle  = gfx::CreateDynamicIndexBuffer(k_maxBatchLineCount * 2, eIndexFormat::U16);

    g_ddCtx.cursor      = Vec3::Zero;
    g_ddCtx.bCursorDown = false;

    // 라인 쉐이더
    {
        ShaderHandle vsHandle = gfx::CreateShader(MemoryView {
            precompiled_shader::DebugDrawLineShader_VS_Bytecode, precompiled_shader::DebugDrawLineShader_VS_Size });

        ShaderHandle psHandle = gfx::CreateShader(MemoryView {
            precompiled_shader::DebugDrawLineShader_PS_Bytecode, precompiled_shader::DebugDrawLineShader_PS_Size });

        g_ddCtx.programHandles[EnumToInt(eDebugDrawProgram::Line)] = gfx::CreateProgram(vsHandle, psHandle, true);
    }

    // 솔리드 쉐이더
    {
        ShaderHandle vsHandle = gfx::CreateShader(MemoryView {
            precompiled_shader::DebugDrawSolidShader_VS_Bytecode, precompiled_shader::DebugDrawSolidShader_VS_Size });

        ShaderHandle psHandle = gfx::CreateShader(MemoryView {
            precompiled_shader::DebugDrawSolidShader_PS_Bytecode, precompiled_shader::DebugDrawSolidShader_PS_Size });

        g_ddCtx.programHandles[EnumToInt(eDebugDrawProgram::Solid)] = gfx::CreateProgram(vsHandle, psHandle, true);
    }

    // 상수 버퍼
    g_ddCtx.cbDebugDrawHandle = gfx::CreateConstantBuffer(sizeof(CB_DEBUGDRAW));

    // 기본 메쉬 생성
    // 박스
    {
        Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Box)];
        mesh       = CreateBoxMesh(Vec3 { 1.f, 1.f, 1.f }, DebugDrawSolidVertex::s_layout);
    }

    // 쿼드
    {
        Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Quad)];
        mesh       = CreatePlaneMesh(Vec2 { 1.f, 1.f }, DebugDrawSolidVertex::s_layout);
    }

    // 콘
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Cone_Low) + i];
            mesh       = CreateCone(1.f, 1.f, k_circleSegmentTable[i], DebugDrawSolidVertex::s_layout);
        }
    }

    // 원
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Circle_Low) + i];
            mesh       = CreateCircle(1.f, k_circleSegmentTable[i], DebugDrawSolidVertex::s_layout);
        }
    }

    // 구
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Sphere_Low) + i];
            mesh       = CreateSphereMesh(1.f, k_sphereSubdivisionTable[i], DebugDrawSolidVertex::s_layout);
        }
    }

    // 실린더
    {
        for (Int32 i = 0; i < EnumCount<eDetailLevel>(); ++i)
        {
            Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Cylinder_Low) + i];
            mesh       = CreateCylinder(1.f, 1.f, k_circleSegmentTable[i], DebugDrawSolidVertex::s_layout);
        }
    }
}

void Shutdown()
{
    g_ddCtx.transformStack.clear();
    g_ddCtx.curTransform = Mat4::Identity;

    g_ddCtx.stateStack.clear();
    g_ddCtx.curState = {};

    // 메시 정리
    for (Mesh& mesh: g_ddCtx.debugDrawMeshes)
    {
        mesh.Unload();
    }

    // 라인 드로우 정리
    g_ddCtx.lineVertices.clear();
    g_ddCtx.lineIndices.clear();

    JAM_DESTROY_VERTEX_BUFFER(g_ddCtx.lineVertexBufferHandle);
    JAM_DESTROY_INDEX_BUFFER(g_ddCtx.lineIndexBufferHandle);

    // 프로그램 정리
    for (ProgramHandle handle: g_ddCtx.programHandles)
    {
        JAM_DESTROY_PROGRAM(handle);
    }

    // 상수 버퍼 정리
    JAM_DESTROY_CONSTANT_BUFFER(g_ddCtx.cbDebugDrawHandle);
}

void Begin()
{
    g_ddCtx.transformStack.clear();
    g_ddCtx.transformStack.push_back(g_ddCtx.curTransform);
    g_ddCtx.curTransform = Mat4::Identity;

    g_ddCtx.stateStack.clear();
    g_ddCtx.stateStack.push_back(g_ddCtx.curState);
    g_ddCtx.curState = {};

    g_ddCtx.lineVertices.clear();
    g_ddCtx.lineIndices.clear();
    g_ddCtx.cursor      = Vec3::Zero;
    g_ddCtx.bCursorDown = false;

    g_ddCtx.curState = {};
}

void End()
{
    g_ddCtx.DrawLineBatch();
}

void PushTransform()
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.transformStack.push_back(g_ddCtx.curTransform);
    g_ddCtx.curTransform = g_ddCtx.transformStack.back();
}

void PopTransform()
{
    g_ddCtx.DrawLineBatch();
    JAM_ASSERT(g_ddCtx.transformStack.size() > 1, "DebugDraw::PopTransform() called with empty stack");
    g_ddCtx.curTransform = g_ddCtx.transformStack.back();
    g_ddCtx.transformStack.pop_back();
}

void SetCamera(const Vec3& _eyePos, const Mat4& _viewProjMat)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.cbDebugDrawCPU.eyePos   = _eyePos;
    g_ddCtx.cbDebugDrawCPU.projView = _viewProjMat.Transpose();
}

void SetTransform(const Mat4& _mat)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * _mat;
}

void SetTranslate(const Vec3& _translate)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * Mat4::CreateTranslation(_translate);
}

void SetRotate(const Quat& _rotate)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * Mat4::CreateFromQuaternion(_rotate);
}

void SetRotate(const Vec3& _axis, const float _angleRad)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * Mat4::CreateFromAxisAngle(_axis, _angleRad);
}

void SetRotate(const Vec3& _yawPitchRollRad)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * Mat4::CreateFromYawPitchRoll(_yawPitchRollRad);
}

void SetScale(const Vec3& _scale)
{
    g_ddCtx.DrawLineBatch();
    const Mat4& parentMat = g_ddCtx.transformStack.back();
    g_ddCtx.curTransform  = parentMat * Mat4::CreateScale(_scale);
}

void MultiplyTransform(const Mat4& _mat)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= _mat;
}

void MultiplyTranslate(const Vec3& _translate)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= Mat4::CreateTranslation(_translate);
}

void MultiplyRotate(const Quat& _rotate)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= Mat4::CreateFromQuaternion(_rotate);
}

void MultiplyRotate(const Vec3& _axis, const float _angleRad)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= Mat4::CreateFromAxisAngle(_axis, _angleRad);
}

void MultiplyRotate(const Vec3& _yawPitchRollRad)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= Mat4::CreateFromYawPitchRoll(_yawPitchRollRad);
}

void MultiplyScale(const Vec3& _scale)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curTransform *= Mat4::CreateScale(_scale);
}

void PushState()
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.stateStack.push_back(g_ddCtx.curState);
    g_ddCtx.curState = g_ddCtx.stateStack.back();
}

void PopState()
{
    g_ddCtx.DrawLineBatch();
    JAM_ASSERT(g_ddCtx.stateStack.size() > 1, "DebugDraw::PopState() called with empty stack");
    g_ddCtx.curState = g_ddCtx.stateStack.back();
    g_ddCtx.stateStack.pop_back();
}

void SetColor(const RGBA _rgba)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curState.color = _rgba;
}

void SetDepthState(const bool _bDepthTest, const bool _bDepthWrite)
{
    g_ddCtx.DrawLineBatch();
    GfxState& state     = g_ddCtx.curState.state;
    state.depthTestFunc = _bDepthTest ? eCompareFunc::Less : eCompareFunc::None;
    state.bDepthWrite   = _bDepthWrite;
}

void SetWireframe(const bool _bWireframe)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curState.bWireframe = _bWireframe;
}

void SetDetailLevel(const eDetailLevel _level)
{
    g_ddCtx.DrawLineBatch();
    g_ddCtx.curState.detailLevel = _level;
}

void MoveTo(const Vec3& _pos)
{
    g_ddCtx.MoveTo(_pos.x, _pos.y, _pos.z);
}

void LineTo(const Vec3& _pos)
{
    g_ddCtx.LineTo(_pos.x, _pos.y, _pos.z);
}

void DrawLine(const Vec3& _from, const Vec3& _to)
{
    g_ddCtx.MoveTo(_from.x, _from.y, _from.z);
    g_ddCtx.LineTo(_to.x, _to.y, _to.z);
}

void DrawArc(const Vec3& _center, const float _radius, const Vec3& _normal, const float _startAngleRad, const float _endAngleRad)
{
    JAM_ASSERT(_radius > 0.f, "Arc radius must be greater than zero");
    JAM_ASSERT(_startAngleRad < _endAngleRad, "Arc start angle must be less than end angle");

    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _normal, Vec3(_radius)));

    Int32 segment   = k_circleSegmentTable[EnumToInt(g_ddCtx.curState.detailLevel)];
    float stepRad   = k_2pi / static_cast<float>(segment);
    Int32 stepCount = static_cast<Int32>(Ceil(Absolute(_endAngleRad - _startAngleRad) / stepRad));

    auto [startSin, startCos] = ComputeSinCos(_startAngleRad);
    g_ddCtx.MoveTo(startCos, 0.f, startSin);

    for (Int32 i = 1; i <= stepCount; ++i)
    {
        float angle     = Min(_startAngleRad + stepRad * static_cast<float>(i), _endAngleRad);
        auto [sin, cos] = ComputeSinCos(angle);
        g_ddCtx.LineTo(cos, 0.f, sin);
    }

    PopTransform();
}

void DrawAABB(const Vec3& _min, const Vec3& _max)
{
    Vec3 size   = _max - _min;
    Vec3 center = 0.5f * (_min + _max);
    debug_draw::DrawOBB(center, size, Vec3::UnitY);
}

void DrawOBB(const Vec3& _center, const Vec3& _size, const Vec3& _normal)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _normal, _size));

    if (g_ddCtx.curState.bWireframe)
    {
        // 앞 사각형
        g_ddCtx.MoveTo(-0.5f, -0.5f, -0.5f);
        g_ddCtx.LineTo(+0.5f, -0.5f, -0.5f);
        g_ddCtx.LineTo(+0.5f, +0.5f, -0.5f);
        g_ddCtx.LineTo(-0.5f, +0.5f, -0.5f);
        g_ddCtx.LineTo(-0.5f, -0.5f, -0.5f);

        // 뒷 사각형
        g_ddCtx.MoveTo(-0.5f, -0.5f, +0.5f);
        g_ddCtx.LineTo(+0.5f, -0.5f, +0.5f);
        g_ddCtx.LineTo(+0.5f, +0.5f, +0.5f);
        g_ddCtx.LineTo(-0.5f, +0.5f, +0.5f);
        g_ddCtx.LineTo(-0.5f, -0.5f, +0.5f);

        // 옆 줄
        g_ddCtx.MoveTo(-0.5f, -0.5f, -0.5f);
        g_ddCtx.LineTo(-0.5f, -0.5f, +0.5f);
        g_ddCtx.MoveTo(+0.5f, -0.5f, -0.5f);
        g_ddCtx.LineTo(+0.5f, -0.5f, +0.5f);
        g_ddCtx.MoveTo(+0.5f, +0.5f, -0.5f);
        g_ddCtx.LineTo(+0.5f, +0.5f, +0.5f);
        g_ddCtx.MoveTo(-0.5f, +0.5f, -0.5f);
        g_ddCtx.LineTo(-0.5f, +0.5f, +0.5f);
    }
    else
    {
        const Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Box)];
        g_ddCtx.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DrawSphere(const Vec3& _center, const float _radius)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, Vec3::UnitY, Vec3(_radius)));

    if (g_ddCtx.curState.bWireframe)
    {
        Int32 circleCount = k_sphereCircleTable[EnumToInt(g_ddCtx.curState.detailLevel)];

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
        eDebugDrawMesh meshType = static_cast<eDebugDrawMesh>(EnumToInt(eDebugDrawMesh::Sphere_Low) + EnumToInt(g_ddCtx.curState.detailLevel));
        const Mesh&    mesh     = g_ddCtx.debugDrawMeshes[EnumToInt(meshType)];
        g_ddCtx.DrawMesh(mesh, false);
    }
    PopTransform();
}

void DrawQuad(const Vec3& _center, const Vec2& _size, const Vec3& _normal)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _normal, Vec3(_size.x, 1.f, _size.y)));

    if (g_ddCtx.curState.bWireframe)
    {
        g_ddCtx.MoveTo(-0.5f, 0.f, -0.5f);
        g_ddCtx.LineTo(+0.5f, 0.f, -0.5f);
        g_ddCtx.LineTo(+0.5f, 0.f, +0.5f);
        g_ddCtx.LineTo(-0.5f, 0.f, +0.5f);
        g_ddCtx.LineTo(-0.5f, 0.f, -0.5f);
    }
    else
    {
        const Mesh& mesh = g_ddCtx.debugDrawMeshes[EnumToInt(eDebugDrawMesh::Quad)];
        g_ddCtx.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DrawGrid(const Vec3& _center, const Vec2& _size, const Vec3& _normal, const float _step)
{
    JAM_ASSERT(_step > 0.f, "Grid step must be greater than zero");

    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _normal, Vec3(1.f)));

    const int halfLineCountX = static_cast<int>(_size.x / _step * 0.5f);
    const int halfLineCountY = static_cast<int>(_size.y / _step * 0.5f);
    for (int i = -halfLineCountX; i <= halfLineCountX; ++i)
    {
        const float x = static_cast<float>(i) * _step;
        g_ddCtx.MoveTo(x, 0.f, static_cast<float>(-halfLineCountY) * _step);
        g_ddCtx.LineTo(x, 0.f, static_cast<float>(+halfLineCountY) * _step);
    }

    for (int i = -halfLineCountY; i <= halfLineCountY; ++i)
    {
        const float z = static_cast<float>(i) * _step;
        g_ddCtx.MoveTo(static_cast<float>(-halfLineCountX) * _step, 0.f, z);
        g_ddCtx.LineTo(static_cast<float>(+halfLineCountX) * _step, 0.f, z);
    }

    PopTransform();
}

void DrawCircle(const Vec3& _center, const float _radius, const Vec3& _normal)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _normal, Vec3(_radius)));

    if (g_ddCtx.curState.bWireframe)
    {
        DrawArc(Vec3(0.f, 0.f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);
    }
    else
    {
        eDebugDrawMesh meshType = static_cast<eDebugDrawMesh>(EnumToInt(eDebugDrawMesh::Circle_Low) + EnumToInt(g_ddCtx.curState.detailLevel));
        const Mesh&    mesh     = g_ddCtx.debugDrawMeshes[EnumToInt(meshType)];
        g_ddCtx.DrawMesh(mesh, false);
    }
}

void DrawCone(const Vec3& _center, const float _height, const float _radius, const Vec3& _dir)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _dir, Vec3(_radius, _height, _radius)));

    if (g_ddCtx.curState.bWireframe)
    {
        // 밑면 원
        DrawArc(Vec3(0.f, -0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);

        // 옆면 선
        Int32 segment = k_circleSegmentTable[EnumToInt(g_ddCtx.curState.detailLevel)];
        float stepRad = k_2pi / static_cast<float>(segment);
        for (Int32 i = 0; i < segment; ++i)
        {
            auto [sin, cos] = ComputeSinCos(i * stepRad);
            g_ddCtx.MoveTo(0.f, +0.5f, 0.f);
            g_ddCtx.LineTo(cos, -0.5f, sin);
        }
    }
    else
    {
        eDebugDrawMesh meshType = static_cast<eDebugDrawMesh>(EnumToInt(eDebugDrawMesh::Cone_Low) + EnumToInt(g_ddCtx.curState.detailLevel));
        const Mesh&    mesh     = g_ddCtx.debugDrawMeshes[EnumToInt(meshType)];
        g_ddCtx.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DrawFrustum(const Vec3& _position, const Vec3& _forward, const Mat4& _projMat)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_position, _forward, Vec3(1.f)));

    // 절두체
    Frustum frustum;
    Frustum::CreateFromMatrix(frustum, _projMat);

    Vec3 corners[8];
    frustum.GetCorners(corners);

    // 근평면
    g_ddCtx.MoveTo(corners[0].x, corners[0].y, corners[0].z);
    g_ddCtx.LineTo(corners[1].x, corners[1].y, corners[1].z);
    g_ddCtx.LineTo(corners[3].x, corners[3].y, corners[3].z);
    g_ddCtx.LineTo(corners[2].x, corners[2].y, corners[2].z);
    g_ddCtx.LineTo(corners[0].x, corners[0].y, corners[0].z);

    // 원평면
    g_ddCtx.MoveTo(corners[4].x, corners[4].y, corners[4].z);
    g_ddCtx.LineTo(corners[5].x, corners[5].y, corners[5].z);
    g_ddCtx.LineTo(corners[7].x, corners[7].y, corners[7].z);
    g_ddCtx.LineTo(corners[6].x, corners[6].y, corners[6].z);
    g_ddCtx.LineTo(corners[4].x, corners[4].y, corners[4].z);

    // 옆면
    g_ddCtx.MoveTo(corners[0].x, corners[0].y, corners[0].z);
    g_ddCtx.LineTo(corners[4].x, corners[4].y, corners[4].z);
    g_ddCtx.MoveTo(corners[1].x, corners[1].y, corners[1].z);
    g_ddCtx.LineTo(corners[5].x, corners[5].y, corners[5].z);
    g_ddCtx.MoveTo(corners[2].x, corners[2].y, corners[2].z);
    g_ddCtx.LineTo(corners[6].x, corners[6].y, corners[6].z);
    g_ddCtx.MoveTo(corners[3].x, corners[3].y, corners[3].z);
    g_ddCtx.LineTo(corners[7].x, corners[7].y, corners[7].z);

    PopTransform();
}

void DrawCylinder(const Vec3& _center, const float _height, const float _radius, const Vec3& _dir)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _dir, Vec3(_radius, _height, _radius)));

    if (g_ddCtx.curState.bWireframe)
    {
        // 위, 아래 원
        DrawArc(Vec3(0.f, -0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);
        DrawArc(Vec3(0.f, +0.5f, 0.f), 1.f, Vec3::Up, 0.f, k_2pi);

        // 옆면 선
        Int32 segment = k_circleSegmentTable[EnumToInt(g_ddCtx.curState.detailLevel)];
        float stepRad = k_2pi / static_cast<float>(segment);
        for (Int32 i = 0; i < segment; ++i)
        {
            auto [sin, cos] = ComputeSinCos(i * stepRad);
            g_ddCtx.MoveTo(cos, -0.5f, sin);
            g_ddCtx.LineTo(cos, +0.5f, sin);
        }
    }
    else
    {
        eDebugDrawMesh meshType = static_cast<eDebugDrawMesh>(EnumToInt(eDebugDrawMesh::Cylinder_Low) + EnumToInt(g_ddCtx.curState.detailLevel));
        const Mesh&    mesh     = g_ddCtx.debugDrawMeshes[EnumToInt(meshType)];
        g_ddCtx.DrawMesh(mesh, false);
    }

    PopTransform();
}

void DrawCapsule(const Vec3& _center, const Vec3& _dir, const float _height, const float _radius)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_center, _dir, Vec3(_radius, _height, _radius)));

    // 원통
    float cylinderHeight = Max(0.f, _height - 2.f * _radius);
    DrawCylinder(Vec3::Zero, cylinderHeight, 1.f, Vec3::UnitY);

    if (g_ddCtx.curState.bWireframe)
    {
        // 반구
        Int32 circleCount = k_sphereCircleTable[EnumToInt(g_ddCtx.curState.detailLevel)];
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

void DrawMesh(const Mesh& _mesh, const Vec3& _position, const Vec3& _forward, const Vec3& _scale)
{
    PushTransform();
    g_ddCtx.SetShapeTransform(CreateWorld(_position, _forward, _scale));
    g_ddCtx.DrawMesh(_mesh, g_ddCtx.curState.bWireframe);
    PopTransform();
}

}   // namespace jam::debug_draw