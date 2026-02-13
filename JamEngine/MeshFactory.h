#pragma once
#include "GraphicsBase.h"
#include "MemoryBlock.h"

namespace Jam
{
class Mesh;

struct TangentFrame
{
    Vector<Vec3> tangents;
    Vector<Vec3> bitangents;
};

[[nodiscard]] Ref<Mesh> CreateCubeMesh(
    Vec3                _size,
    const VertexLayout& _vl);

[[nodiscard]] Ref<Mesh> CreateSphereMesh(
    float               _radius,
    Int32               _divisionLevel,
    const VertexLayout& _vl);

[[nodiscard]] Ref<Mesh> CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _vl);

[[nodiscard]] Ref<Mesh> CreateCircle(
    float               _radius,
    Int32               _numTri,
    const VertexLayout& _vl);

[[nodiscard]] Ref<Mesh> CreateCone(
    float               _height,
    float               _radius,
    Int32               _numTri,
    const VertexLayout& _vl);

[[nodiscard]] Ref<Mesh> CreateCylinder(
    float               _height,
    float               _radius,
    Int32               _numTries,
    const VertexLayout& _vl);

[[nodiscard]] Vector<Vec3> ComputeNormals(
    Span<const Vec3> _positions,
    CMemoryBlock     _idxData,
    bool             _bUseU32);

[[nodiscard]] TangentFrame ComputeTangentFrame(
    Span<const Vec3> _positions,
    Span<const Vec3> _normals,
    Span<const Vec2> _texCoords,
    CMemoryBlock     _idxData,
    bool             _bUseU32);

}   // namespace Jam