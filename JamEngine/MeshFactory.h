#pragma once
#include "GraphicsBase.h"
#include "MemView.h"
#include "Mesh.h"

namespace jam
{

struct TangentFrame
{
    std::vector<Vec3> tangents;
    std::vector<Vec3> bitangents;
};

[[nodiscard]] Mesh CreateCubeMesh(
    Vec3                _size,
    const VertexLayout& _vl);

[[nodiscard]] Mesh CreateSphereMesh(
    float               _radius,
    Int32               _divisionLevel,
    const VertexLayout& _vl);

[[nodiscard]] Mesh CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _vl);

[[nodiscard]] Mesh CreateCircle(
    float               _radius,
    Int32               _numTri,
    const VertexLayout& _vl);

[[nodiscard]] Mesh CreateCone(
    float               _height,
    float               _radius,
    Int32               _numTri,
    const VertexLayout& _vl);

[[nodiscard]] Mesh CreateCylinder(
    float               _height,
    float               _radius,
    Int32               _numTri,
    const VertexLayout& _vl);

[[nodiscard]] std::vector<Vec3> ComputeNormal(
    std::span<const Vec3> _positions,
    ConstMemView          _indexData,
    bool                  _bUseU32);

[[nodiscard]] TangentFrame ComputeTangentFrame(
    std::span<const Vec3> _positions,
    std::span<const Vec3> _normals,
    std::span<const Vec2> _texCoords,
    ConstMemView          _indexData,
    bool                  _bUseU32);

}   // namespace jam