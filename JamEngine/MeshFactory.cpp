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
    const std::span<const Vec3> positions,
    const std::span<const Vec2> texCoords,
    const ConstMemView          indexData,
    const bool                  bUseU32,
    const VertexLayout&         layout)
{
    // validation
    JAM_ASSERT(layout.Has(eVertexAttribute::Position), "Position attribute is required in the vertex layout.");
    JAM_ASSERT(!positions.empty(), "Position data is empty.");
    JAM_ASSERT(!indexData.IsValid() || indexData.GetByteWidth() > 0, "Index data is empty.");

    // check required attributes
    const bool hasNormal    = layout.Has(eVertexAttribute::Normal);
    const bool hasTangent   = layout.Has(eVertexAttribute::Tangent);
    const bool hasBitangent = layout.Has(eVertexAttribute::Bitangent);
    const bool hasTexCoord  = layout.Has(eVertexAttribute::TexCoord0);

    // compute normals if needed
    std::vector<Vec3> normals;
    if (hasNormal)
    {
        normals = ComputeNormal(positions, indexData, bUseU32);
    }

    // compute tangents and bitangents if needed
    TangentFrame tangentFrame;
    if (hasTangent || hasBitangent)
    {
        JAM_ASSERT(hasTexCoord, "TexCoord0 attribute is required to compute tangents and bitangents.");
        JAM_ASSERT(!texCoords.empty(), "TexCoord0 data is required to compute tangents and bitangents.");
        JAM_ASSERT(!normals.empty(), "Normals must be computed before computing tangents and bitangents.");

        tangentFrame = ComputeTangentFrame(positions, normals, texCoords, indexData, bUseU32);
    }

    // allocate vertex buffer
    const Int32 numVerts   = NarrowCast<Int32>(positions.size());
    const Int32 vertStride = layout.GetStride();

    std::vector<Byte> verts;
    verts.resize(numVerts * vertStride);

    // write position data
    {
        const VertexLayoutElement& elem = layout.GetElement(eVertexAttribute::Position);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Byte* pDst = verts.data() + vertStride * i + elem.offset;
            std::memcpy(pDst, &positions[i], sizeof(Vec3));
        }
    }

    // write normal data
    if (hasNormal)
    {
        const VertexLayoutElement& elem = layout.GetElement(eVertexAttribute::Normal);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Byte* pDst = verts.data() + vertStride * i + elem.offset;
            std::memcpy(pDst, &normals[i], sizeof(Vec3));
        }
    }

    // write tangent data
    if (hasTangent)
    {
        const VertexLayoutElement& elem = layout.GetElement(eVertexAttribute::Tangent);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Byte* pDst = verts.data() + vertStride * i + elem.offset;
            std::memcpy(pDst, &tangentFrame.tangents[i], sizeof(Vec3));
        }
    }

    // write bitangent data
    if (hasBitangent)
    {
        const VertexLayoutElement& elem = layout.GetElement(eVertexAttribute::Bitangent);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Byte* pDst = verts.data() + vertStride * i + elem.offset;
            std::memcpy(pDst, &tangentFrame.bitangents[i], sizeof(Vec3));
        }
    }

    // write texcoord data
    if (hasTexCoord)
    {
        const VertexLayoutElement& elem = layout.GetElement(eVertexAttribute::TexCoord0);
        for (Int32 i = 0; i < numVerts; ++i)
        {
            Byte* pDst = verts.data() + vertStride * i + elem.offset;
            std::memcpy(pDst, &texCoords[i], sizeof(Vec2));
        }
    }

    // create and return mesh
    Mesh mesh;
    mesh.LoadFromMemory(verts, layout, indexData, bUseU32);
    return mesh;
}

std::vector<Vec3> ComputeNormals(
    const std::span<const Vec3> _positions,
    const ConstMemView          _indices,
    const bool                  _bUseU32) noexcept
{
    using namespace DirectX;

    const size_t numVerts  = _positions.size();
    const Int32  idxStride = GetIndexStride(_bUseU32);
    const size_t numFaces  = Div(_indices.GetByteWidth(), 3ull * idxStride);
    const Byte*  pIndices  = _indices.GetDataPtr();

    // accumulate weighted normals per vertex
    std::vector<Vec3> accumulatedNormals(numVerts, Vec3::Zero);

    for (size_t face = 0; face < numFaces; ++face)
    {
        const Byte* pFaceIndices = &pIndices[face * 3 * idxStride];

        // read triangle indices
        UInt32 i0, i1, i2;
        if (_bUseU32)
        {
            i0 = *reinterpret_cast<const UInt32*>(pFaceIndices + 0 * idxStride);
            i1 = *reinterpret_cast<const UInt32*>(pFaceIndices + 1 * idxStride);
            i2 = *reinterpret_cast<const UInt32*>(pFaceIndices + 2 * idxStride);
        }
        else
        {
            i0 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pFaceIndices + 0 * idxStride));
            i1 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pFaceIndices + 1 * idxStride));
            i2 = static_cast<UInt32>(*reinterpret_cast<const UInt16*>(pFaceIndices + 2 * idxStride));
        }

        JAM_ASSERT(i0 < numVerts && i1 < numVerts && i2 < numVerts, "Index out of range.");

        // get triangle positions
        const Vec3 p0 = _positions[i0];
        const Vec3 p1 = _positions[i1];
        const Vec3 p2 = _positions[i2];

        // calculate face normal
        const Vec3 edge1      = Normalized(p1 - p0);
        const Vec3 edge2      = Normalized(p2 - p0);
        const Vec3 faceNormal = Normalized(Cross(edge1, edge2));

        // calculate angle weights for each vertex
        const float angle0 = Acos(Clamp(Dot(edge1, edge2), -1.0f, 1.0f));

        const Vec3  edge3  = Normalized(p2 - p1);
        const Vec3  edge4  = Normalized(p0 - p1);
        const float angle1 = Acos(Clamp(Dot(edge3, edge4), -1.0f, 1.0f));

        const Vec3  edge5  = Normalized(p0 - p2);
        const Vec3  edge6  = Normalized(p1 - p2);
        const float angle2 = Acos(Clamp(Dot(edge5, edge6), -1.0f, 1.0f));

        // accumulate angle-weighted face normals
        accumulatedNormals[i0] += faceNormal * angle0;
        accumulatedNormals[i1] += faceNormal * angle1;
        accumulatedNormals[i2] += faceNormal * angle2;
    }

    // normalize accumulated normals
    std::vector<Vec3> normals;
    normals.reserve(numVerts);

    for (size_t vert = 0; vert < numVerts; ++vert)
    {
        normals.emplace_back(Normalized(accumulatedNormals[vert]));
    }

    return normals;
}

std::vector<Vec3> ComputeNormal(
    const std::span<const Vec3> _positions,
    const ConstMemView          _indexData,
    const bool                  _bUseU32)
{
    return ComputeNormals(_positions, _indexData, _bUseU32);
}

TangentFrame ComputeTangentFrame(
    const std::span<const Vec3> _positions,
    const std::span<const Vec3> _normals,
    const std::span<const Vec2> _texCoords,
    const ConstMemView          _indexData,
    const bool                  _bUseU32)
{
    using namespace DirectX;

    TangentFrame result;

    const size_t numVerts  = _positions.size();
    const Int32  idxStride = GetIndexStride(_bUseU32);
    const size_t numFaces  = Div(_indexData.GetByteWidth(), 3ull * idxStride);
    const Byte*  pIndices  = _indexData.GetDataPtr();

    // initialize result vectors
    result.tangents.resize(numVerts, Vec3::Zero);
    result.bitangents.resize(numVerts, Vec3::Zero);

    // temporary accumulation vectors
    std::vector<Vec3> tan1(numVerts, Vec3::Zero);
    std::vector<Vec3> tan2(numVerts, Vec3::Zero);

    // accumulate tangent space vectors for each face
    for (size_t face = 0; face < numFaces; ++face)
    {
        const Byte* pFaceIndices = &pIndices[face * 3 * idxStride];

        // read triangle indices
        UInt32 i0, i1, i2;
        if (_bUseU32)
        {
            i0 = *reinterpret_cast<const UInt32*>(pFaceIndices + 0 * idxStride);
            i1 = *reinterpret_cast<const UInt32*>(pFaceIndices + 1 * idxStride);
            i2 = *reinterpret_cast<const UInt32*>(pFaceIndices + 2 * idxStride);
        }
        else
        {
            i0 = *reinterpret_cast<const UInt16*>(pFaceIndices + 0 * idxStride);
            i1 = *reinterpret_cast<const UInt16*>(pFaceIndices + 1 * idxStride);
            i2 = *reinterpret_cast<const UInt16*>(pFaceIndices + 2 * idxStride);
        }

        JAM_ASSERT(i0 < numVerts && i1 < numVerts && i2 < numVerts, "Index out of range.");

        // get triangle data
        Vec3 v0 = _positions[i0];
        Vec3 v1 = _positions[i1];
        Vec3 v2 = _positions[i2];

        const Vec2& w0 = _texCoords[i0];
        const Vec2& w1 = _texCoords[i1];
        const Vec2& w2 = _texCoords[i2];

        // calculate position deltas
        const float x1 = v1.x - v0.x;
        const float x2 = v2.x - v0.x;
        const float y1 = v1.y - v0.y;
        const float y2 = v2.y - v0.y;
        const float z1 = v1.z - v0.z;
        const float z2 = v2.z - v0.z;

        // calculate uv deltas
        const float s1 = w1.x - w0.x;
        const float s2 = w2.x - w0.x;
        const float t1 = w1.y - w0.y;
        const float t2 = w2.y - w0.y;

        // calculate determinant and inverse
        float det = s1 * t2 - s2 * t1;
        det       = IsZeroApprox(det) ? 1.0f : 1.0f / det;

        // calculate tangent and bitangent directions
        const Vec3 sdir {
            (t2 * x1 - t1 * x2) * det,
            (t2 * y1 - t1 * y2) * det,
            (t2 * z1 - t1 * z2) * det
        };

        const Vec3 tdir {
            (s1 * x2 - s2 * x1) * det,
            (s1 * y2 - s2 * y1) * det,
            (s1 * z2 - s2 * z1) * det
        };

        // accumulate for all vertices of the triangle
        tan1[i0] += sdir;
        tan1[i1] += sdir;
        tan1[i2] += sdir;

        tan2[i0] += tdir;
        tan2[i1] += tdir;
        tan2[i2] += tdir;
    }

    // orthogonalize and calculate final tangent frame
    for (size_t i = 0; i < numVerts; ++i)
    {
        Vec3 n = _normals[i];
        Vec3 t = tan1[i];

        // gram-schmidt orthogonalize tangent with respect to normal
        Vec3 tangent = Normalized(t - n * Dot(n, t));

        // handle degenerate case
        if (IsZeroApprox(tangent))
        {
            // choose an arbitrary perpendicular axis
            Vec3 axis;
            if (n.x < n.y && n.x < n.z)
            {
                axis = g_XMIdentityR0;   // x axis
            }
            else if (n.y < n.z)
            {
                axis = g_XMIdentityR1;   // y axis
            }
            else
            {
                axis = g_XMIdentityR2;   // z axis
            }

            tangent = Normalized(Cross(n, axis));
        }

        // calculate bitangent and determine handedness
        Vec3 bitangent = Normalized(Cross(n, tangent));

        const float handedness = Dot(Cross(n, tangent), tan2[i]);
        if (handedness < 0.0f)
        {
            bitangent = -bitangent;
        }

        result.tangents[i]   = tangent;
        result.bitangents[i] = bitangent;
    }

    return result;
}

Mesh CreateCubeMesh(const Vec3 _size, const VertexLayout& _vl)
{
    const float hWidth  = _size.x * 0.5f;
    const float hHeight = _size.y * 0.5f;
    const float hDepth  = _size.z * 0.5f;

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
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // front
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // back
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // left
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // right
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // top
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f }, // bottom
    };

    constexpr UInt16 k_indices[] = {
        0, 1, 2, 0, 2, 3,   // front
        4,
        5,
        6,
        4,
        6,
        7,   // back
        8,
        9,
        10,
        8,
        10,
        11,   // left
        12,
        13,
        14,
        12,
        14,
        15,   // right
        16,
        17,
        18,
        16,
        18,
        19,   // top
        20,
        21,
        22,
        20,
        22,
        23,   // bottom
    };

    return CreateMeshHelper(positions, k_texcoords, k_indices, false, _vl);
}

Mesh CreateSphereMesh(const float _radius, const Int32 _divisionLevel, const VertexLayout& _vl)
{
    // base icosahedron vertices
    constexpr float k_t = (1.0f + Sqrt(5.0f)) / 2.0f;

    constexpr Vec3 k_positions[] = {
        { -1.f, +k_t,  0.f },
        { +1.f, +k_t,  0.f },
        { -1.f, -k_t,  0.f },
        { +1.f, -k_t,  0.f },
        {  0.f, -1.f, +k_t },
        {  0.f, +1.f, +k_t },
        {  0.f, -1.f, -k_t },
        {  0.f, +1.f, -k_t },
        { +k_t,  0.f, -1.f },
        { +k_t,  0.f, +1.f },
        { -k_t,  0.f, -1.f },
        { -k_t,  0.f, +1.f },
    };

    constexpr Vec2 k_texCoords[] = {
        { 0.00f, 0.00f },
        { 1.00f, 0.00f },
        { 0.00f, 1.00f },
        { 1.00f, 1.00f },
        { 0.50f, 0.00f },
        { 0.50f, 1.00f },
        { 0.00f, 0.50f },
        { 1.00f, 0.50f },
        { 0.25f, 0.25f },
        { 0.75f, 0.25f },
        { 0.25f, 0.75f },
        { 0.75f, 0.75f },
    };

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
        // subdivide mesh
        LoopSubdivisionOutput output = LoopSubdivide(k_positions, k_texCoords, k_indices, false, _divisionLevel);

        // normalize and scale to sphere radius
        for (Vec3& pos: output.positions)
        {
            pos.Normalize();
            pos *= _radius;
        }

        return CreateMeshHelper(output.positions, output.texcoords, output.indices, output.bUseU32, _vl);
    }

    // return base icosahedron
    return CreateMeshHelper(k_positions, k_texCoords, k_indices, false, _vl);
}

Mesh CreatePlaneMesh(const Vec2& _size, const VertexLayout& _vl)
{
    const float hWidth  = _size.x * 0.5f;
    const float hHeight = _size.y * 0.5f;

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

    constexpr UInt16 k_indices[] = { 0, 1, 2, 0, 2, 3 };

    return CreateMeshHelper(positions, k_texcoords, k_indices, false, _vl);
}

Mesh CreateCircle(const float _radius, const Int32 _numTri, const VertexLayout& _vl)
{
    JAM_ASSERT(_numTri >= 6, "Circle must have at least 6 triangles.");

    const Int32 numVerts = _numTri + 1;   // triangles + center vertex
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "Too many triangles for circle.");

    std::vector<Vec3> positions(numVerts);
    std::vector<Vec2> texcoords(numVerts);

    // center vertex
    positions[0] = Vec3::Zero;
    texcoords[0] = Vec2 { 0.5f, 0.5f };

    // circle vertices
    const float step = k_2pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad   = static_cast<float>(i) * step;
        const auto [s, c] = SinCosApprox(rad);
        const float x     = c * _radius;
        const float z     = s * _radius;

        positions[i + 1] = Vec3 { x, 0.f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // generate indices
    std::vector<UInt16> indices(_numTri * 3);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        indices[i * 3 + 0] = 0;                       // center
        indices[i * 3 + 1] = (i + 1) % _numTri + 1;   // next vertex
        indices[i * 3 + 2] = i + 1;                   // current vertex
    }

    return CreateMeshHelper(positions, texcoords, indices, false, _vl);
}

Mesh CreateCone(const float _height, const float _radius, const Int32 _numTri, const VertexLayout& _vl)
{
    JAM_ASSERT(_numTri >= 6, "Cone base must have at least 6 triangles.");

    const Int32 numVerts = _numTri + 2;   // base circle + apex + base center
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "Too many triangles for cone.");

    std::vector<Vec3> position(numVerts);
    std::vector<Vec2> texcoords(numVerts);

    // apex vertex
    position[0]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[0] = Vec2 { 0.5f, 0.f };

    // base circle vertices
    const float step = k_2pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const auto [s, c] = SinCosApprox(static_cast<float>(i) * step);
        const float x     = c * _radius;
        const float z     = s * _radius;

        position[i + 1]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // base center vertex
    const Int32 bottomCenterIndex = numVerts - 1;
    position[bottomCenterIndex]   = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[bottomCenterIndex]  = Vec2 { 0.5f, 0.5f };

    // generate indices
    std::vector<UInt16> indices(_numTri * 6);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const Int32 nextIdx = (i + 1) % _numTri + 1;
        const Int32 currIdx = i + 1;

        // side triangle
        indices[i * 6 + 0] = 0;         // apex
        indices[i * 6 + 1] = nextIdx;   // next base vertex
        indices[i * 6 + 2] = currIdx;   // current base vertex

        // base triangle
        indices[i * 6 + 3] = bottomCenterIndex;   // base center
        indices[i * 6 + 4] = currIdx;             // current base vertex
        indices[i * 6 + 5] = nextIdx;             // next base vertex
    }

    return CreateMeshHelper(position, texcoords, indices, false, _vl);
}

Mesh CreateCylinder(const float _height, const float _radius, const Int32 _numTri, const VertexLayout& _vl)
{
    JAM_ASSERT(_numTri >= 6, "Cylinder must have at least 6 triangles.");

    const Int32 numVerts = _numTri * 2 + 2;   // top + bottom circles + 2 centers
    JAM_ASSERT(std::cmp_less_equal(numVerts, k_uint16Max), "Too many triangles for cylinder.");

    std::vector<Vec3> position(numVerts);
    std::vector<Vec2> texcoords(numVerts);

    const float step = k_2pi / static_cast<float>(_numTri);

    // top circle vertices
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const auto [s, c] = SinCosApprox(static_cast<float>(i) * step);
        const float x     = c * _radius;
        const float z     = s * _radius;

        position[i]  = Vec3 { x, _height * 0.5f, z };
        texcoords[i] = Vec2 { static_cast<float>(i) / static_cast<float>(_numTri), 1.f };
    }

    // bottom circle vertices
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const auto [s, c] = SinCosApprox(static_cast<float>(i) * step);
        const float x     = c * _radius;
        const float z     = s * _radius;

        position[i + _numTri]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + _numTri] = Vec2 { static_cast<float>(i) / static_cast<float>(_numTri), 0.f };
    }

    // center vertices
    const Int32 topCircleCenterIdx = _numTri * 2;
    const Int32 botCircleCenterIdx = _numTri * 2 + 1;

    position[topCircleCenterIdx]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[topCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    position[botCircleCenterIdx]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[botCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    // generate indices
    std::vector<UInt16> indices(_numTri * 12);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const Int32 nextIdx = (i + 1) % _numTri;
        const Int32 topCurr = i;
        const Int32 topNext = nextIdx;
        const Int32 botCurr = i + _numTri;
        const Int32 botNext = nextIdx + _numTri;

        // side triangles (2 per segment)
        indices[i * 12 + 0] = topCurr;
        indices[i * 12 + 1] = topNext;
        indices[i * 12 + 2] = botCurr;

        indices[i * 12 + 3] = topNext;
        indices[i * 12 + 4] = botNext;
        indices[i * 12 + 5] = botCurr;

        // top cap triangle
        indices[i * 12 + 6] = topCircleCenterIdx;
        indices[i * 12 + 7] = topNext;
        indices[i * 12 + 8] = topCurr;

        // bottom cap triangle
        indices[i * 12 + 9]  = botCircleCenterIdx;
        indices[i * 12 + 10] = botCurr;
        indices[i * 12 + 11] = botNext;
    }

    return CreateMeshHelper(position, texcoords, indices, false, _vl);
}

}   // namespace jam
