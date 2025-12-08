#pragma once
#include "GraphicsBase.h"
#include "Mesh.h"
#include "MemView.h"

namespace jam
{

struct TangentAndBitangent
{
    std::vector<Vec3> tangents;
    std::vector<Vec3> bitangents;
};

[[nodiscard]] Mesh CreateCubeMesh(Vec3 _size, const VertexLayout& _layout);
[[nodiscard]] Mesh CreateSphereMesh(float _radius, Int32 _divisionLevel, const VertexLayout& _layout);
[[nodiscard]] Mesh CreatePlaneMesh(const Vec2& _size, const VertexLayout& _layout);
[[nodiscard]] Mesh CreateCircle(float _radius, Int32 _numTri, const VertexLayout& _vl);
[[nodiscard]] Mesh CreateCone(float _height, float _radius, Int32 _numTri, const VertexLayout& _layout);
[[nodiscard]] Mesh CreateCylinder(float _height, float _radius, Int32 _numTri, const VertexLayout& _layout);

[[nodiscard]] std::vector<Vec3>   ComputeNormal(std::span<const Vec3> _positions, ConstMemView _indexData, eIndexFormat _format);
[[nodiscard]] TangentAndBitangent ComputeTangentAndBitangent(std::span<const Vec3> _positions, std::span<const Vec3> _normals, std::span<const Vec2> _texCoords, ConstMemView _indexData, eIndexFormat _format);

}   // namespace jam