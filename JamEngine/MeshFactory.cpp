#include "pch.h"

#include "MeshFactory.h"

#include "BaseUtilities.h"
#include "GfxBase.h"
#include "LoopSubdivision.h"
#include "Memory.h"
#include "Mesh.h"

#include <DirectXMesh.h>
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

JAM_NODISCARD static Mesh CreateMeshHelper(
    const std::span<const Vec3> _positions,
    const std::span<const Vec2> _texCoords,
    const MemoryView           _indexData,
    const eIndexFormat          _indexFormat,
    const VertexLayout&         _layout)
{
    // 노말 계산
    std::vector<Vec3> normals;
    if (_layout.Has(eVertexAttribute::Normal))
    {
        normals = ComputeNormal(_positions, _indexData, _indexFormat);
    }

    // 탄젠트, 바이탄젠트 계산
    std::vector<Vec3> tangents;
    std::vector<Vec3> bitangents;
    if (_layout.Has(eVertexAttribute::Tangent) || _layout.Has(eVertexAttribute::Bitangent))
    {
        TangentAndBitangent tanBitan = ComputeTangentAndBitangent(_positions, normals, _texCoords, _indexData, _indexFormat);
        tangents                     = std::move(tanBitan.tangents);
        bitangents                   = std::move(tanBitan.bitangents);
    }

    // 메쉬 생성
    CreateMeshInput input;
    input.positionData        = MemoryView { _positions };
    input.normalDataOrNull    = MemoryView { normals };
    input.tangentDataOrNull   = MemoryView { tangents };
    input.bitangentDataOrNull = MemoryView { bitangents };
    input.texcoord0DataOrNull = MemoryView { _texCoords };
    input.indexData           = _indexData;
    input.indexFormat         = _indexFormat;
    return CreateMesh(input, _layout);
}

////////////////////////////////////////////////////////////

namespace jam
{

Mesh CreateBoxMesh(
    const Vec3&         _size,
    const VertexLayout& _layout)
{
    float hWidth  = _size.x * 0.5f;
    float hHeight = _size.y * 0.5f;
    float hDepth  = _size.z * 0.5f;

    Vec3 positions[] = {
        // front face
        { -hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { -hWidth, +hHeight, -hDepth },
        // back face
        { +hWidth, -hHeight, +hDepth },
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        // left face
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, -hHeight, -hDepth },
        { -hWidth, +hHeight, -hDepth },
        { -hWidth, +hHeight, +hDepth },
        // right face
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, -hDepth },
        // top face
        { -hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        // bottom face
        { -hWidth, -hHeight, +hDepth },
        { +hWidth, -hHeight, +hDepth },
        { +hWidth, -hHeight, -hDepth },
        { -hWidth, -hHeight, -hDepth },
    };

    constexpr Vec2 k_texcoords[] = {
        // front face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
        // back face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
        // left face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
        // right face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
        // top face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
        // bottom face
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
    };

    constexpr UInt16 k_indices[] = {
        // front face
        0,
        1,
        2,
        0,
        2,
        3,
        // back face
        4,
        5,
        6,
        4,
        6,
        7,
        // left face
        8,
        9,
        10,
        8,
        10,
        11,
        // right face
        12,
        13,
        14,
        12,
        14,
        15,
        // top face
        16,
        17,
        18,
        16,
        18,
        19,
        // bottom face
        20,
        21,
        22,
        20,
        22,
        23,
    };

    return CreateMeshHelper(positions, k_texcoords, MemoryView { k_indices }, eIndexFormat::U16, _layout);
}

Mesh CreateSphereMesh(
    const float         _radius,
    const Int32         _divisionLevel,
    const VertexLayout& _layout)
{
    // 버텍스 정의
    constexpr float k_t = (1.0f + Sqrt(5.0f)) / 2.0f;

    constexpr Vec3 k_positions[] = {
        { -1.f,  k_t,  0.f },
        {  1.f,  k_t,  0.f },
        { -1.f, -k_t,  0.f },
        {  1.f, -k_t,  0.f },
        {  0.f, -1.f,  k_t },
        {  0.f,  1.f,  k_t },
        {  0.f, -1.f, -k_t },
        {  0.f,  1.f, -k_t },
        {  k_t,  0.f, -1.f },
        {  k_t,  0.f,  1.f },
        { -k_t,  0.f, -1.f },
        { -k_t,  0.f,  1.f },
    };

    // 텍스처 좌표
    constexpr Vec2 k_texCoords[] = {
        {  0.0f,  0.0f },
        {  1.0f,  0.0f },
        {  0.0f,  1.0f },
        {  1.0f,  1.0f },
        {  0.5f,  0.0f },
        {  0.5f,  1.0f },
        {  0.0f,  0.5f },
        {  1.0f,  0.5f },
        { 0.25f, 0.25f },
        { 0.75f, 0.25f },
        { 0.25f, 0.75f },
        { 0.75f, 0.75f },
    };

    // 인덱스 정의
    constexpr UInt16 k_indices[] = {
        0,
        11,
        5,
        0,
        5,
        1,
        0,
        1,
        7,
        0,
        7,
        10,
        0,
        10,
        11,
        1,
        5,
        9,
        5,
        11,
        4,
        11,
        10,
        2,
        10,
        7,
        6,
        7,
        1,
        8,
        3,
        9,
        4,
        3,
        4,
        2,
        3,
        2,
        6,
        3,
        6,
        8,
        3,
        8,
        9,
        4,
        9,
        5,
        2,
        4,
        11,
        6,
        2,
        10,
        8,
        6,
        7,
        9,
        8,
        1,
    };

    // 서브 디비전
    LoopSubdivisionOutput output = LoopSubdivide(k_positions, k_texCoords, MemoryView { k_indices }, eIndexFormat::U16, _divisionLevel);

    // 위치 정규화 및 반지름 적용
    for (Vec3& pos: output.positions)
    {
        pos.Normalize();
        pos *= _radius;
    }

    return CreateMeshHelper(output.positions, output.texcoords, MemoryView { output.indices }, output.indexFormat, _layout);
}

Mesh CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _layout)
{
    float hWidth  = _size.x * 0.5f;
    float hHeight = _size.y * 0.5f;

    const Vec3 positions[] = {
        { -hWidth, 0.f, -hHeight },
        { +hWidth, 0.f, -hHeight },
        { +hWidth, 0.f, +hHeight },
        { -hWidth, 0.f, +hHeight },
    };

    constexpr Vec2 k_texcoords[] = {
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 1.f, 0.f },
        { 0.f, 0.f },
    };

    constexpr UInt16 k_indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    return CreateMeshHelper(positions, k_texcoords, MemoryView { k_indices }, eIndexFormat::U16, _layout);
}

Mesh CreateCircle(
    const float         _radius,
    const Int32         _triangleCount,
    const VertexLayout& _layout)
{
    JAM_ASSERT(_triangleCount >= 6, "원은 최소 6개의 삼각형으로 구성되어야 합니다.");

    Int32 vertexCount = _triangleCount + 1;   // 삼각형 개수 + 중심 정점
    JAM_ASSERT(std::cmp_less_equal(vertexCount, UInt16Limits::max()), "원기둥의 삼각형 개수가 너무 많습니다.");

    std::vector<Vec3> positions;
    positions.resize(vertexCount);

    std::vector<Vec2> texcoords;
    texcoords.resize(vertexCount);

    // 중심 정점
    positions[0] = Vec3::Zero;
    texcoords[0] = Vec2 { 0.5f, 0.5f };

    float step = k_2pi / static_cast<float>(_triangleCount);
    for (Int32 i = 0; i < _triangleCount; ++i)
    {
        auto [sin, cos]  = ComputeSinCos(i * step);
        float x          = cos * _radius;
        float z          = sin * _radius;
        positions[i + 1] = Vec3 { x, 0.f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_triangleCount * 3);
    for (Int32 i = 0; i < _triangleCount; ++i)
    {
        indices[i * 3 + 0] = 0;                              // 중심 정점
        indices[i * 3 + 1] = (i + 1) % _triangleCount + 1;   // 다음 원 정점
        indices[i * 3 + 2] = i + 1;                          // 현재 원 정점
    }

    return CreateMeshHelper(positions, texcoords, MemoryView { indices }, eIndexFormat::U16, _layout);
}

Mesh CreateCone(
    const float         _height,
    const float         _radius,
    const Int32         _bottomTriangleCount,
    const VertexLayout& _layout)
{
    JAM_ASSERT(_bottomTriangleCount >= 6, "원뿔의 바닥은 최소 6개의 삼각형으로 구성되어야 합니다.");

    // 정점 생성
    Int32 vertexCount = _bottomTriangleCount + 2;   // 바닥 원의 정점 + 꼭지점 + 바닥 중심 정점
    JAM_ASSERT(std::cmp_less_equal(vertexCount, UInt16Limits::max()), "원기둥의 삼각형 개수가 너무 많습니다.");

    std::vector<Vec3> position;
    position.resize(vertexCount);

    std::vector<Vec2> texcoords;
    texcoords.resize(vertexCount);

    // 꼭지점
    position[0]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[0] = Vec2 { 0.5f, 0.f };

    // 바닥 원의 정점
    float step = k_2pi / static_cast<float>(_bottomTriangleCount);
    for (Int32 i = 0; i < _bottomTriangleCount; ++i)
    {
        auto [sin, cos]  = ComputeSinCos(i * step);
        const float x    = cos * _radius;
        const float z    = sin * _radius;
        position[i + 1]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // 바닥 중심 정점
    Int32 bottomCenterIndex      = vertexCount - 1;
    position[bottomCenterIndex]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[bottomCenterIndex] = Vec2 { 0.5f, 0.5f };

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_bottomTriangleCount * 6);   // 옆면 + 바닥
    for (Int32 i = 0; i < _bottomTriangleCount; ++i)
    {
        // 옆면 삼각형
        indices[i * 6 + 0] = 0;                                    // 꼭지점
        indices[i * 6 + 1] = (i + 1) % _bottomTriangleCount + 1;   // 다음 바닥 원 정점
        indices[i * 6 + 2] = i + 1;                                // 현재 바닥 원 정점

        // 바닥 삼각형
        indices[i * 6 + 3] = bottomCenterIndex;
        indices[i * 6 + 4] = i + 1;
        indices[i * 6 + 5] = (i + 1) % _bottomTriangleCount + 1;
    }

    return CreateMeshHelper(position, texcoords, MemoryView { indices }, eIndexFormat::U16, _layout);
}

Mesh CreateCylinder(
    const float         _height,
    const float         _radius,
    const Int32         _bottomTriangleCount,
    const VertexLayout& _layout)
{
    JAM_ASSERT(_bottomTriangleCount >= 6, "원기둥은 최소 6개의 삼각형으로 구성되어야 합니다.");

    // 정점 생성
    Int32 vertexCount = _bottomTriangleCount * 2 + 2;   // 위/아래 원의 정점 + 위/아래 중심 정점
    JAM_ASSERT(std::cmp_less_equal(vertexCount, UInt16Limits::max()), "원기둥의 삼각형 개수가 너무 많습니다.");

    std::vector<Vec3> position;
    position.resize(vertexCount);

    std::vector<Vec2> texcoords;
    texcoords.resize(vertexCount);

    float step = k_2pi / static_cast<float>(_bottomTriangleCount);

    // 아래 원의 정점
    for (Int32 i = 0; i < _bottomTriangleCount; ++i)
    {
        auto [sin, cos] = ComputeSinCos(i * step);
        float x         = cos * _radius;
        float z         = sin * _radius;
        position[i]     = Vec3 { x, -_height * 0.5f, z };
        texcoords[i]    = Vec2 { static_cast<float>(i) / static_cast<float>(_bottomTriangleCount), 1.f };
    }

    // 위 원의 정점
    for (Int32 i = 0; i < _bottomTriangleCount; ++i)
    {
        auto [sin, cos]                     = ComputeSinCos(i * step);
        float x                             = cos * _radius;
        float z                             = sin * _radius;
        position[i + _bottomTriangleCount]  = Vec3 { x, _height * 0.5f, z };
        texcoords[i + _bottomTriangleCount] = Vec2 { static_cast<float>(i) / static_cast<float>(_bottomTriangleCount), 0.f };
    }

    // 아래/위 중심 정점
    Int32 bottomCenterIndex = _bottomTriangleCount * 2;
    Int32 topCenterIndex    = _bottomTriangleCount * 2 + 1;

    position[bottomCenterIndex]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[bottomCenterIndex] = Vec2 { 0.5f, 0.5f };

    position[topCenterIndex]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[topCenterIndex] = Vec2 { 0.5f, 0.5f };

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_bottomTriangleCount * 12);   // 옆면 + 위/아래
    for (Int32 i = 0; i < _bottomTriangleCount; ++i)
    {
        // 옆면 삼각형 2개
        indices[i * 12 + 0] = i;                                                       // 아래 원 현재 정점
        indices[i * 12 + 1] = (i + 1) % _bottomTriangleCount;                          // 아래 원 다음 정점
        indices[i * 12 + 2] = i + _bottomTriangleCount;                                // 위 원 현재 정점
        indices[i * 12 + 3] = (i + 1) % _bottomTriangleCount;                          // 아래 원 다음 정점
        indices[i * 12 + 4] = (i + 1) % _bottomTriangleCount + _bottomTriangleCount;   // 위 원 다음 정점
        indices[i * 12 + 5] = i + _bottomTriangleCount;                                // 위 원 현재 정점

        // 아래 원 삼각형
        indices[i * 12 + 6] = bottomCenterIndex;
        indices[i * 12 + 7] = (i + 1) % _bottomTriangleCount;
        indices[i * 12 + 8] = i;

        // 위 원 삼각형
        indices[i * 12 + 9]  = topCenterIndex;
        indices[i * 12 + 10] = i + _bottomTriangleCount;
        indices[i * 12 + 11] = (i + 1) % _bottomTriangleCount + _bottomTriangleCount;
    }

    return CreateMeshHelper(position, texcoords, MemoryView { indices }, eIndexFormat::U16, _layout);
}

std::vector<Vec3> ComputeNormal(
    const std::span<const Vec3> _positions,
    const MemoryView           _indexData,
    const eIndexFormat          _format)
{
    std::vector<Vec3> normals;
    normals.resize(_positions.size());

    if (_format == eIndexFormat::U16)
    {
        const UInt16* indices = reinterpret_cast<const UInt16*>(_indexData.GetDataPtr());
        const Int32   count   = NarrowCast<Int32>(_indexData.GetByteWidth() / sizeof(UInt16));
        JAM_CHECK_COM(DirectX::ComputeNormals(indices, count / 3, _positions.data(), _positions.size(), DirectX::CNORM_WIND_CW, normals.data()));
    }
    else
    {
        const UInt32* indices = reinterpret_cast<const UInt32*>(_indexData.GetDataPtr());
        const Int32   count   = NarrowCast<Int32>(_indexData.GetByteWidth() / sizeof(UInt32));
        JAM_CHECK_COM(DirectX::ComputeNormals(indices, count / 3, _positions.data(), _positions.size(), DirectX::CNORM_WIND_CW, normals.data()));
    }

    return normals;
}

TangentAndBitangent ComputeTangentAndBitangent(
    const std::span<const Vec3> _positions,
    const std::span<const Vec3> _normals,
    const std::span<const Vec2> _texCoords,
    const MemoryView           _indexData,
    const eIndexFormat          _format)
{
    TangentAndBitangent result;
    result.tangents.resize(_positions.size());
    result.bitangents.resize(_positions.size());

    if (_format == eIndexFormat::U16)
    {
        const UInt16* indices = reinterpret_cast<const UInt16*>(_indexData.GetDataPtr());
        const Int32   count   = NarrowCast<Int32>(_indexData.GetByteWidth() / sizeof(UInt16));
        JAM_CHECK_COM(DirectX::ComputeTangentFrame(indices, count / 3, _positions.data(), _normals.data(), _texCoords.data(), _positions.size(), result.tangents.data(), result.bitangents.data()));
    }
    else
    {
        const UInt32* indices = reinterpret_cast<const UInt32*>(_indexData.GetDataPtr());
        const Int32   count   = NarrowCast<Int32>(_indexData.GetByteWidth() / sizeof(UInt32));
        JAM_CHECK_COM(DirectX::ComputeTangentFrame(indices, count / 3, _positions.data(), _normals.data(), _texCoords.data(), _positions.size(), result.tangents.data(), result.bitangents.data()));
    }

    return result;
}

Mesh CreateMesh(const CreateMeshInput& _input, const VertexLayout& _layout)
{
    JAM_ASSERT(_input.positionData.GetByteWidth(), "position data is essential");
    JAM_ASSERT(_input.indexData.IsValid(), "index data is essential");
    JAM_ASSERT(_layout.Has(eVertexAttribute::Position), "position attribute is essential");

    const MemoryView* mems[EnumCount<eVertexAttribute>()] = {
        &_input.positionData,          // Position
        &_input.normalDataOrNull,      // Normal
        &_input.tangentDataOrNull,     // Tangent
        &_input.bitangentDataOrNull,   // Bitangent
        &_input.color0DataOrNull,      // Color0
        &_input.color1DataOrNull,      // Color1
        &_input.color2DataOrNull,      // Color2
        &_input.color3DataOrNull,      // Color3
        &_input.texcoord0DataOrNull,   // TexCoord0
        &_input.texcoord1DataOrNull,   // TexCoord1
        &_input.texcoord2DataOrNull,   // TexCoord2
        &_input.texcoord3DataOrNull,   // TexCoord3
    };

    // 버텍스 cpu 버퍼 생성
    const VertexLayoutElement& posElement = _layout.GetElement(eVertexAttribute::Position);

    Int32 vertexDataByteWidth = NarrowCast<Int32>(_input.positionData.GetByteWidth());
    Int32 vertexStride        = _layout.GetStride();
    Int32 vertexCount         = vertexDataByteWidth / posElement.byteWidth;

    std::vector<Byte> vertexData;
    vertexData.resize(vertexCount * vertexStride);
    Byte* pVertexData = vertexData.data();

    // 버텍스 데이터 채우기
    for (eVertexAttribute attrib: EnumRange<eVertexAttribute>())
    {
        const MemoryView* pMem = mems[EnumToInt(attrib)];
        if (_layout.Has(attrib) && pMem->IsValid())
        {
            const VertexLayoutElement& elem = _layout.GetElement(attrib);
            for (Int32 i = 0; i < vertexCount; ++i)
            {
                Byte*       pDst = pVertexData + i * vertexStride + elem.offset;
                const Byte* pSrc = pMem->GetDataPtr() + i * elem.byteWidth;
                std::memcpy(pDst, pSrc, elem.byteWidth);
            }
        }
    }

    // 인덱스
    Mesh mesh;
    mesh.Load(MemoryView { vertexData }, _layout, MemoryView { _input.indexData }, _input.indexFormat);
    return mesh;
}

}   // namespace jam
