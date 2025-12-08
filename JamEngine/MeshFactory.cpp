#include "pch.h"

#include "MeshFactory.h"

#include "BaseUtilities.h"
#include "GraphicsBase.h"
#include "LoopSubdivision.h"
#include "MemView.h"
#include "Mesh.h"

namespace jam
{

[[nodiscard]] static Mesh CreateMeshHelper(
    const std::span<const Vec3> _positions,
    const std::span<const Vec2> _texCoords,
    const ConstMemView       _indexData,
    const eIndexFormat          _indexFormat,
    const VertexLayout&         _layout)
{
    JAM_ASSERT(_layout.Has(eVertexAttribute::Position), "Position attribute is required in the vertex layout.");
    JAM_ASSERT(!_positions.empty(), "Position data is empty.");
    JAM_ASSERT(!_indexData.IsValid() || _indexData.GetByteWidth() > 0, "Index data is empty.");

    bool bHasNoraml    = _layout.Has(eVertexAttribute::Normal);
    bool bHasTangent   = _layout.Has(eVertexAttribute::Tangent);
    bool bHasBitangent = _layout.Has(eVertexAttribute::Bitangent);
    bool bHasTexCoord  = _layout.Has(eVertexAttribute::TexCoord0);

    // 노말 계산
    std::vector<Vec3> normals;
    if (bHasNoraml)
    {
        normals = ComputeNormal(_positions, _indexData, _indexFormat);
    }

    // 탄젠트, 바이탄젠트 계산
    TangentAndBitangent tanBitan;
    if (bHasTangent || bHasBitangent)
    {
        JAM_ASSERT(bHasTexCoord, "TexCoord0 attribute is required to compute tangents and bitangents.");
        JAM_ASSERT(!_texCoords.empty(), "TexCoord0 data is required to compute tangents and bitangents.");
        JAM_ASSERT(!normals.empty(), "Normals must be computed before computing tangents and bitangents.");
        tanBitan = ComputeTangentAndBitangent(_positions, normals, _texCoords, _indexData, _indexFormat);
    }

    // 버텍스 버퍼 생성
    const Int32       numVerts     = NarrowCast<Int32>(_positions.size());
    const Int32       vertexStride = _layout.GetStride();
    std::vector<Byte> vertexData;
    vertexData.resize(numVerts * vertexStride);

    // position
    const VertexLayoutElement& posElem = _layout.GetElement(eVertexAttribute::Position);
    for (Int32 i = 0; i < numVerts; ++i)
    {
        Vec3  pos  = _positions[i];
        Byte* pDst = vertexData.data() + vertexStride * i + posElem.offset;
        std::memcpy(pDst, &pos, posElem.byteWidth);
    }

    // normal
    if (bHasNoraml)
    {
        const VertexLayoutElement& normElem = _layout.GetElement(eVertexAttribute::Normal);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Vec3  norm = normals[i];
            Byte* pDst = vertexData.data() + vertexStride * i + normElem.offset;
            std::memcpy(pDst, &norm, normElem.byteWidth);
        }
    }

    // tangent
    if (bHasTangent)
    {
        const VertexLayoutElement& tanElem = _layout.GetElement(eVertexAttribute::Tangent);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Vec3  tan  = tanBitan.tangents[i];
            Byte* pDst = vertexData.data() + vertexStride * i + tanElem.offset;
            std::memcpy(pDst, &tan, tanElem.byteWidth);
        }
    }

    // bitangent
    if (bHasBitangent)
    {
        const VertexLayoutElement& bitanElem = _layout.GetElement(eVertexAttribute::Bitangent);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Vec3  bitan = tanBitan.bitangents[i];
            Byte* pDst  = vertexData.data() + vertexStride * i + bitanElem.offset;
            std::memcpy(pDst, &bitan, bitanElem.byteWidth);
        }
    }

    // texcoord
    if (bHasTexCoord)
    {
        const VertexLayoutElement& texElem = _layout.GetElement(eVertexAttribute::TexCoord0);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            const Vec2& tex  = _texCoords[i];
            Byte*       pDst = vertexData.data() + vertexStride * i + texElem.offset;
            std::memcpy(pDst, &tex, texElem.byteWidth);
        }
    }

    Mesh mesh;
    mesh.LoadFromMemory(vertexData, _layout, _indexData, _indexFormat);
    return mesh;
}

std::vector<Vec3> ComputeNormals(
    const std::span<const Vec3> _positions,
    const ConstMemView       _indices,
    const eIndexFormat          _indexFormat) noexcept
{
    using namespace DirectX;

    const size_t numVerts  = _positions.size();
    const Int32  idxStride = GetIndexFormatByte(_indexFormat);
    const size_t numFaces  = Div(_indices.GetByteWidth(), 3ull * idxStride);
    const Byte*  pIndices  = _indices.GetDataPtr();

    std::vector<Vec3> vertNormals(numVerts, Vec3::Zero);
    for (size_t face = 0; face < numFaces; ++face)
    {
        const Byte* pHeader = &pIndices[face * 3 * idxStride];
        UInt32      i0, i1, i2;

        // 인덱스 읽기
        if (_indexFormat == eIndexFormat::U16)
        {
            i0 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 0 * idxStride));
            i1 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 1 * idxStride));
            i2 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 2 * idxStride));
        }
        else
        {
            i0 = *reinterpret_cast<const UInt32*>(pHeader + 0 * idxStride);
            i1 = *reinterpret_cast<const UInt32*>(pHeader + 1 * idxStride);
            i2 = *reinterpret_cast<const UInt32*>(pHeader + 2 * idxStride);
        }

        JAM_ASSERT(i0 < numVerts && i1 < numVerts && i2 < numVerts, "index number is out of range.");

        const Vec3 p0 = _positions[i0];
        const Vec3 p1 = _positions[i1];
        const Vec3 p2 = _positions[i2];

        const Vec3 u          = Normalized(p1 - p0);
        const Vec3 v          = Normalized(p2 - p0);
        const Vec3 faceNormal = Normalized(Cross(u, v));

        float cosAngle0 = Dot(u, v);
        cosAngle0       = Clamp(cosAngle0, -1.0f, 1.0f);
        const float w0  = Acos(cosAngle0);

        const Vec3 c = Normalized(p2 - p1);
        const Vec3 d = Normalized(p0 - p1);

        const float cosAngle1 = Clamp(Dot(c, d), -1.0f, 1.0f);
        const float w1        = Acos(cosAngle1);

        const Vec3 e = Normalized(p0 - p2);
        const Vec3 f = Normalized(p1 - p2);

        const float cosAngle2 = Clamp(Dot(e, f), -1.0f, 1.0f);
        const float w2        = Acos(cosAngle2);

        vertNormals[i0] += faceNormal * w0;
        vertNormals[i1] += faceNormal * w1;
        vertNormals[i2] += faceNormal * w2;
    }

    std::vector<Vec3> normals;
    normals.reserve(numVerts);
    for (size_t vert = 0; vert < numVerts; ++vert)
    {
        const Vec3 normal = Normalized(vertNormals[vert]);
        normals.push_back(normal);
    }

    return normals;
}

TangentAndBitangent ComputeTangentFrame(
    ConstMemView       _indices,
    eIndexFormat          _indexFormat,
    std::span<const Vec3> _positions,
    std::span<const Vec3> _normals,
    std::span<const Vec2> _texcoords)
{
    using namespace DirectX;

    TangentAndBitangent result;
    std::vector<Vec3>&  tangents   = result.tangents;
    std::vector<Vec3>&  bitangents = result.bitangents;

    const size_t numVerts  = _positions.size();
    const Int32  idxStride = GetIndexFormatByte(_indexFormat);
    const size_t numFaces  = Div(_indices.GetByteWidth(), 3ull * idxStride);

    tangents.resize(numVerts, Vec3::Zero);
    bitangents.resize(numVerts, Vec3::Zero);

    std::vector<Vec3> tan1(numVerts, Vec3::Zero);
    std::vector<Vec3> tan2(numVerts, Vec3::Zero);

    for (size_t face = 0; face < numFaces; ++face)
    {
        const Byte* pHeader = &_indices.GetDataPtr()[face * 3 * idxStride];
        UInt32      i0, i1, i2;

        if (_indexFormat == eIndexFormat::U16)
        {
            i0 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 0 * idxStride));
            i1 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 1 * idxStride));
            i2 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pHeader + 2 * idxStride));
        }
        else
        {
            i0 = *reinterpret_cast<const UInt32*>(pHeader + 0 * idxStride);
            i1 = *reinterpret_cast<const UInt32*>(pHeader + 1 * idxStride);
            i2 = *reinterpret_cast<const UInt32*>(pHeader + 2 * idxStride);
        }

        JAM_ASSERT(i0 < numVerts && i1 < numVerts && i2 < numVerts, "index number is out of range.");

        Vec3 v0 = _positions[i0];
        Vec3 v1 = _positions[i1];
        Vec3 v2 = _positions[i2];

        const Vec2& w0 = _texcoords[i0];
        const Vec2& w1 = _texcoords[i1];
        const Vec2& w2 = _texcoords[i2];

        // Position deltas
        float x1 = v1.x - v0.x;
        float x2 = v2.x - v0.x;
        float y1 = v1.y - v0.y;
        float y2 = v2.y - v0.y;
        float z1 = v1.z - v0.z;
        float z2 = v2.z - v0.z;

        // UV deltas
        float s1 = w1.x - w0.x;
        float s2 = w2.x - w0.x;
        float t1 = w1.y - w0.y;
        float t2 = w2.y - w0.y;

        // Calculate determinant
        float r = s1 * t2 - s2 * t1;

        // Avoid division by zero
        if (IsZeroApprox(r))
        {
            r = 1.0f;
        }
        else
        {
            r = 1.0f / r;
        }

        // calculate tangent and bitangent
        const Vec3 sdir {
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        };

        const Vec3 tdir {
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        };

        tan1[i0] += sdir;
        tan1[i1] += sdir;
        tan1[i2] += sdir;

        tan2[i0] += tdir;
        tan2[i1] += tdir;
        tan2[i2] += tdir;
    }

    // orthogonalize and store results
    for (size_t i = 0; i < numVerts; ++i)
    {
        const Vec3 n = _normals[i];
        const Vec3 t = tan1[i];

        // orthogonalize
        Vec3 tan = Normalized(t - n * Dot(n, t));
        if (IsZeroApprox(tan))
        {
            Vec3 axis;
            if (n.x < n.y && n.x < n.z)
            {
                axis = g_XMIdentityR0;   // X axis
            }
            else if (n.y < n.z)
            {
                axis = g_XMIdentityR1;   // Y axis
            }
            else
            {
                axis = g_XMIdentityR2;   // Z axis
            }

            tan = Normalized(Cross(n, axis));
        }

        const Vec3 t2    = tan2[i];
        Vec3       bitan = Normalized(Cross(n, tan));

        float handedness = Dot(Cross(n, tan), t2);
        if (handedness < 0.0f)
        {
            bitan = -bitan;
        }

        tangents[i]   = tan;
        bitangents[i] = bitan;
    }

    return result;
}

}   // namespace jam

namespace jam
{
Mesh CreateCubeMesh(
    const Vec3          _size,
    const VertexLayout& _layout)
{
    float hWidth  = _size.x * 0.5f;
    float hHeight = _size.y * 0.5f;
    float hDepth  = _size.z * 0.5f;

    Vec3 positions[] = {
        // front face
        { -hWidth, -hHeight, -hDepth },
        { -hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { +hWidth, -hHeight, -hDepth },
        // back face
        { +hWidth, -hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        { -hWidth, -hHeight, +hDepth },
        // left face
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        { -hWidth, +hHeight, -hDepth },
        { -hWidth, -hHeight, -hDepth },
        // right face
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, +hDepth },
        { +hWidth, -hHeight, +hDepth },
        // top face
        { -hWidth, +hHeight, -hDepth },
        { -hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, -hDepth },
        // bottom face
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, +hDepth },
    };

    constexpr Vec2 k_texcoords[] = {
        // front face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        // back face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        // left face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        // right face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        // top face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        // bottom face
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
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

    return CreateMeshHelper(positions, k_texcoords, k_indices, eIndexFormat::U16, _layout);
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

    if (_divisionLevel > 0)
    {
        // 서브 디비전
        LoopSubdivisionOutput output = LoopSubdivide(k_positions, k_texCoords, k_indices, eIndexFormat::U16, _divisionLevel);

        // 위치 정규화 및 반지름 적용
        for (Vec3& pos: output.positions)
        {
            pos.Normalize();
            pos *= _radius;
        }

        return CreateMeshHelper(output.positions, output.texcoords, output.indices, output.indexFormat, _layout);
    }
    else   // 정이십면체
    {
        return CreateMeshHelper(k_positions, k_texCoords, k_indices, eIndexFormat::U16, _layout);
    }
}

Mesh CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _layout)
{
    float hWidth  = _size.x * 0.5f;
    float hHeight = _size.y * 0.5f;

    const Vec3 positions[] = {
        { -hWidth, 0.f, -hHeight },
        { -hWidth, 0.f, +hHeight },
        { +hWidth, 0.f, +hHeight },
        { +hWidth, 0.f, -hHeight },
    };

    constexpr Vec2 k_texcoords[] = {
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
    };

    constexpr UInt16 k_indices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
    };

    return CreateMeshHelper(positions, k_texcoords, k_indices, eIndexFormat::U16, _layout);
}

Mesh CreateCircle(
    const float         _radius,
    const Int32         _numTri,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_numTri >= 6, "원은 최소 6개의 삼각형으로 구성되어야 합니다.");

    const Int32 numVerts = _numTri + 1;   // 삼각형 개수 + 중심 정점
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "원기둥의 삼Triangles 개수가 너무 많습니다.");

    std::vector<Vec3> positions;
    positions.resize(numVerts);

    std::vector<Vec2> texcoords;
    texcoords.resize(numVerts);

    // 중심 정점
    positions[0] = Vec3::Zero;
    texcoords[0] = Vec2 { 0.5f, 0.5f };

    float step = k_2pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad       = static_cast<float>(i) * step;
        const auto [sin, cos] = SinCosApprox(rad);
        const float x         = cos * _radius;
        const float z         = sin * _radius;
        positions[i + 1]      = Vec3 { x, 0.f, z };
        texcoords[i + 1]      = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_numTri * 3);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        indices[i * 3 + 0] = 0;                       // 중심 정점
        indices[i * 3 + 1] = (i + 1) % _numTri + 1;   // 다음 원 정점
        indices[i * 3 + 2] = i + 1;                   // 현재 원 정점
    }

    return CreateMeshHelper(positions, texcoords, indices, eIndexFormat::U16, _vl);
}

Mesh CreateCone(
    const float         _height,
    const float         _radius,
    const Int32         _numTri,
    const VertexLayout& _layout)
{
    JAM_ASSERT(_numTri >= 6, "원뿔의 바닥은 최소 6개의 삼角형으로 구성되어야 합니다.");

    // 정점 생성
    Int32 numVerts = _numTri + 2;   // 바닥 원의 정점 + 꼭지점 + 바닥 중심 정점
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "원기둥의 삼각형 개수가 너무 많습니다.");

    std::vector<Vec3> position;
    position.resize(numVerts);

    std::vector<Vec2> texcoords;
    texcoords.resize(numVerts);

    // 꼭지점
    position[0]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[0] = Vec2 { 0.5f, 0.f };

    // 바닥 원의 정점
    float step = k_2pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        auto [sin, cos]  = SinCosApprox(static_cast<float>(i) * step);
        const float x    = cos * _radius;
        const float z    = sin * _radius;
        position[i + 1]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // 바닥 중심 정점
    Int32 bottomCenterIndex      = numVerts - 1;
    position[bottomCenterIndex]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[bottomCenterIndex] = Vec2 { 0.5f, 0.5f };

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_numTri * 6);   // 옆면 + 바닥
    for (Int32 i = 0; i < _numTri; ++i)
    {
        // 옆면 삼각형
        indices[i * 6 + 0] = 0;                       // 꼭지점
        indices[i * 6 + 1] = (i + 1) % _numTri + 1;   // 다음 바닥 원 정점
        indices[i * 6 + 2] = i + 1;                   // 현재 바닥 원 정점

        // 바닥 삼각형
        indices[i * 6 + 3] = bottomCenterIndex;
        indices[i * 6 + 4] = i + 1;
        indices[i * 6 + 5] = (i + 1) % _numTri + 1;
    }

    return CreateMeshHelper(position, texcoords, indices, eIndexFormat::U16, _layout);
}

Mesh CreateCylinder(
    const float         _height,
    const float         _radius,
    const Int32         _numTri,
    const VertexLayout& _layout)
{
    JAM_ASSERT(_numTri >= 6, "원기둥은 최소 6개의 삼각형으로 구성되어야 합니다.");

    // 정점 생성
    Int32 numVerts = _numTri * 2 + 2;
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "원기둥의 삼각형 개수가 너무 많습니다.");

    std::vector<Vec3> position;
    position.resize(numVerts);

    std::vector<Vec2> texcoords;
    texcoords.resize(numVerts);

    float step = k_2pi / static_cast<float>(_numTri);

    // 위 원
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad       = static_cast<float>(i) * step;
        const auto [sin, cos] = SinCosApprox(rad);
        const float x         = cos * _radius;
        const float z         = sin * _radius;
        position[i]           = Vec3 { x, _height * 0.5f, z };
        texcoords[i]          = Vec2 { static_cast<float>(i) / static_cast<float>(_numTri), 1.f };
    }

    // 아래 원
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad          = static_cast<float>(i) * step;
        const auto [sin, cos]    = SinCosApprox(rad);
        const float x            = cos * _radius;
        const float z            = sin * _radius;
        position[i + _numTri]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + _numTri] = Vec2 { static_cast<float>(i) / static_cast<float>(_numTri), 0.f };
    }

    // 위/아래 중심 정점
    Int32 topCircleCenterIdx = _numTri * 2;
    Int32 botCircleCenterIdx = _numTri * 2 + 1;

    position[topCircleCenterIdx]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[topCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    position[botCircleCenterIdx]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[botCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    // 인덱스 생성
    std::vector<UInt16> indices;
    indices.resize(_numTri * 12);   // 옆면 + 위/아래
    for (Int32 i = 0; i < _numTri; ++i)
    {
        // 옆면 삼각형 2개
        indices[i * 12 + 0] = i;                                 // 위 원 현재 정점
        indices[i * 12 + 1] = (i + 1) % _numTri;               // 위 원 다음 정점
        indices[i * 12 + 2] = i + _numTri;                     // 아래 원 현재 정점
        indices[i * 12 + 3] = (i + 1) % _numTri;               // 위 원 다음 정점
        indices[i * 12 + 4] = (i + 1) % _numTri + _numTri;   // 아래 원 다음 정점
        indices[i * 12 + 5] = i + _numTri;                     // 아래 원 현재 정점

        // 위 원 삼각형 (중심에서 바깥쪽으로, CCW)
        indices[i * 12 + 6] = topCircleCenterIdx;
        indices[i * 12 + 7] = (i + 1) % _numTri;
        indices[i * 12 + 8] = i;

        // 아래 원 삼각형 (중심에서 바깥쪽으로, CCW)
        indices[i * 12 + 9]  = botCircleCenterIdx;
        indices[i * 12 + 10] = i + _numTri;
        indices[i * 12 + 11] = (i + 1) % _numTri + _numTri;
    }

    return CreateMeshHelper(position, texcoords, indices, eIndexFormat::U16, _layout);
}

std::vector<Vec3> ComputeNormal(
    const std::span<const Vec3> _positions,
    const ConstMemView       _indexData,
    const eIndexFormat          _format)
{
    return ComputeNormals(_positions, _indexData, _format);
}

TangentAndBitangent ComputeTangentAndBitangent(
    const std::span<const Vec3> _positions,
    const std::span<const Vec3> _normals,
    const std::span<const Vec2> _texCoords,
    const ConstMemView       _indexData,
    const eIndexFormat          _format)
{
    return ComputeTangentFrame(_indexData, _format, _positions, _normals, _texCoords);
}

}   // namespace jam
