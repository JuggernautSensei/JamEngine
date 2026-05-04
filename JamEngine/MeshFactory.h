#pragma once
#include "GraphicsBase.h"
#include "MemoryView.h"

namespace Jam
{

class Mesh;

enum class eCreateConeMesh
{
    OriginAtCenter,
    OriginAtBase,
    OriginAtApex,
};

enum class eCreateCylinderMesh
{
    OriginAtCenter,
    OriginAtBase,
    OriginAtTop,
};

[[nodiscard]] Ref<Mesh> CreateCubeMesh(
    Vec3                _size,
    const VertexLayout& _attribLayoutOrEmpty = {});

[[nodiscard]] Ref<Mesh> CreateSphereMesh(
    float               _radius,
    Int32               _divisionLevel,
    const VertexLayout& _attribLayoutOrEmpty = {});

// _normal은 평면의 법선
// _uv0, _uv1은 평면의 왼쪽 위, 오른쪽 아래에 대응하는 텍스처 좌표
[[nodiscard]] Ref<Mesh> CreatePlaneMesh(
    Vec2                _size,
    Vec2                _uv0,
    Vec2                _uv1,
    Vec3                _normal,
    const VertexLayout& _attribLayoutOrEmpty = {});

[[nodiscard]] Ref<Mesh> CreatePlaneMesh(
    Vec2                _size,
    Vec3                _normal,
    const VertexLayout& _attribLayoutOrEmpty = {});

[[nodiscard]] Ref<Mesh> CreateCircleMesh(
    float               _radius,
    Vec3                _normal,
    Int32               _numTri,
    const VertexLayout& _attribLayoutOrEmpty = {});

// _normal은 밑 원의 법선
[[nodiscard]] Ref<Mesh> CreateConeMesh(
    float               _height,
    float               _radius,
    Vec3                _normal,
    Int32               _numTri,
    eCreateConeMesh     _option,
    const VertexLayout& _attribLayoutOrEmpty = {});

// _normal은 밑 원의 법선
[[nodiscard]] Ref<Mesh> CreateCylinderMesh(
    float               _height,
    float               _radius,
    Vec3                _normal,
    Int32               _numTries,
    eCreateCylinderMesh _option,
    const VertexLayout& _attribLayoutOrEmpty = {});

[[nodiscard]] Vector<Vec3> ComputeNormals(
    Span<const Vec3> _positions,
    MemoryView       _indexData,
    bool             _bUseU32);

struct TangentFrame
{
    Vector<Vec3> tangents;
    Vector<Vec3> bitangents;
};

[[nodiscard]] TangentFrame ComputeTangentFrame(
    Span<const Vec3> _positions,
    Span<const Vec3> _normals,
    Span<const Vec2> _texCoords,
    MemoryView       _indexData,
    bool             _bUseU32);

}   // namespace Jam