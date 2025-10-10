#pragma once
#include "GfxBase.h"
#include "Memory.h"

namespace jam
{

class Mesh;
class VertexLayout;

/// @brief 박스 메쉬 생성
/// @param _size 박스의 가로, 세로, 깊이 크기
/// @param _layout 정점 레이아웃
/// (0, 0, 0)을 중심으로 하여 (-size.x/2, -size.y/2, -size.z/2) ~ (size.x/2, size.y/2, size.z/2) 범위에 생성됨.
JAM_NODISCARD Mesh CreateBoxMesh(
    const Vec3&         _size,
    const VertexLayout& _layout);

/// @brief 구 메쉬 생성
/// @param _radius 구의 반지름
/// @param _divisionLevel 구의 세분화 레벨. 높을수록 정점과 삼각형이 많아짐.
///        0은 정이십면체, 1씩 증가할 때마다 삼각형이 4배씩 증가함.
///        너무 큰 값을 주지 않도록 주의.
/// @param _layout 정점 레이아웃
/// (0, 0, 0)을 중심으로 하여 반지름이 _radius인 구가 생성됨.
JAM_NODISCARD Mesh CreateSphereMesh(
    float               _radius,
    Int32               _divisionLevel,
    const VertexLayout& _layout);

/// @brief 평면 메쉬 생성
/// @param _size 평면의 가로, 세로 크기
/// @param _layout 정점 레이아웃
/// (0, 0, 0)을 중심으로 하여 (-size.x/2, 0, -size.y/2) ~ (size.x/2, 0, size.y/2) 범위에 생성됨.
JAM_NODISCARD Mesh CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _layout);

/// @brief 원 메쉬 생성
/// @param _radius 원의 반지름
/// @param _triangleCount 원을 구성하는 삼각형의 개수 (최소 6)
/// @param _layout 정점 레이아웃
/// (0, 0, 0)을 중심으로 하여 반지름이 _radius인 원이 생성됨.
JAM_NODISCARD Mesh CreateCircle(
    float               _radius,
    Int32               _triangleCount,
    const VertexLayout& _layout);

/// @brief 원뿔 메쉬 생성
/// @param _height 밑면에서 꼭대기까지의 높이
/// @param _radius 밑면의 반지름
/// @param _bottomTriangleCount 밑면을 구성하는 삼각형의 개수 (최소 6)
/// @param _layout 정점 레이아웃
/// (0, 0, 0)을 밑면 중심으로 하여 y축 방향으로 높이가 _height인 원뿔이 생성됨.
JAM_NODISCARD Mesh CreateCone(
    float               _height,
    float               _radius,
    Int32               _bottomTriangleCount,
    const VertexLayout& _layout);

/// @brief 원기둥 메쉬 생성
/// @param _height 원기둥의 높이
/// @param _radius 원기둥의 반지름
/// @param _bottomTriangleCount 원기둥을 구성하는 삼각형의 개수 (최소 6)
/// @param _layout 정점 레이아웃
///
JAM_NODISCARD Mesh CreateCylinder(
    float               _height,
    float               _radius,
    Int32               _bottomTriangleCount,
    const VertexLayout& _layout);

///////////////////////////////////
///          UTILITIES          ///
///////////////////////////////////

JAM_NODISCARD std::vector<Vec3> ComputeNormal(
    std::span<const Vec3> _positions,
    MemoryView     _indexData,
    eIndexFormat          _format);

struct TangentAndBitangent
{
    std::vector<Vec3> tangents;
    std::vector<Vec3> bitangents;
};

JAM_NODISCARD TangentAndBitangent ComputeTangentAndBitangent(
    std::span<const Vec3> _positions,
    std::span<const Vec3> _normals,
    std::span<const Vec2> _texCoords,
    MemoryView     _indexData,
    eIndexFormat          _format);

struct CreateMeshInput
{
    MemoryView positionData        = {};   // essential
    MemoryView normalDataOrNull    = {};
    MemoryView tangentDataOrNull   = {};
    MemoryView bitangentDataOrNull = {};
    MemoryView texcoord0DataOrNull = {};
    MemoryView texcoord1DataOrNull = {};
    MemoryView texcoord2DataOrNull = {};
    MemoryView texcoord3DataOrNull = {};
    MemoryView color0DataOrNull    = {};
    MemoryView color1DataOrNull    = {};
    MemoryView color2DataOrNull    = {};
    MemoryView color3DataOrNull    = {};

    MemoryView   indexData   = {};   // essential
    eIndexFormat indexFormat = eIndexFormat::U32;
};

JAM_NODISCARD Mesh CreateMesh(const CreateMeshInput& _input, const VertexLayout& _layout);

}   // namespace jam