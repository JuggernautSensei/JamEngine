#include "pch.h"

#include "MeshFactory.h"

#include "GeometryUtilities.h"
#include "GraphicsBase.h"
#include "LoopSubdivision.h"
#include "Mesh.h"

namespace Jam
{

namespace
{

    [[nodiscard]] Ref<Mesh> CreateMesh_(
        const Span<const Vec3> _positions,
        const Span<const Vec2> _texCoordsOrEmpty,
        const VertexLayout&    _attribLayoutOrEmpty,
        const MemoryView       _idxData,
        const bool             _bUseU32)
    {
        // validation
        JAM_ASSERT(!_positions.empty(), "Position data is empty.");
        JAM_ASSERT(!_idxData.IsEmpty(), "Index data is empty.");
        JAM_ASSERT(!_attribLayoutOrEmpty.Has(eVertexAttribute::Position), "Position attribute should not be included in the vertex layout, as it is provided separately.");

        // 레이아웃 요구 체크
        const bool bPosOnly      = _attribLayoutOrEmpty.IsEmpty();
        const bool bHasTexCoord  = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord0);
        const bool bHasNormal    = _attribLayoutOrEmpty.Has(eVertexAttribute::Normal);
        const bool bHasTangent   = _attribLayoutOrEmpty.Has(eVertexAttribute::Tangent);
        const bool bHasBitangent = _attribLayoutOrEmpty.Has(eVertexAttribute::Bitangent);

        // 노멀 계산
        Vector<Vec3> normals = {};
        if (bHasNormal)
        {
            normals = ComputeNormals(_positions, _idxData, _bUseU32);
        }

        // 탄젠트 프레임 계산
        TangentFrame tangentFrame = {};
        if (bHasTangent || bHasBitangent)
        {
            JAM_ASSERT(bHasTexCoord, "TexCoord0 attribute is required to compute tangents and bitangents.");
            JAM_ASSERT(!_texCoordsOrEmpty.empty(), "TexCoord0 data is required to compute tangents and bitangents.");
            JAM_ASSERT(!normals.empty(), "Normals must be computed before computing tangents and bitangents.");
            tangentFrame = ComputeTangentFrame(_positions, normals, _texCoordsOrEmpty, _idxData, _bUseU32);
        }

        if (bPosOnly)
        {
            return Mesh::Load(_positions, _idxData);
        }
        else
        {
            const SizeT  numVerts = _positions.size();
            Vector<Byte> attribData(_attribLayoutOrEmpty.GetStride() * numVerts);

            // 버텍스 작성
            VertexWriter vertWriter { attribData, _attribLayoutOrEmpty };
            for (SizeT i = 0; i < numVerts; ++i)
            {
                // write normal
                if (bHasNormal)
                {
                    vertWriter.WriteAt(i, eVertexAttribute::Normal, normals[i]);
                }

                // write tangent
                if (bHasTangent)
                {
                    vertWriter.WriteAt(i, eVertexAttribute::Tangent, tangentFrame.tangents[i]);
                }

                // write bitangent
                if (bHasBitangent)
                {
                    vertWriter.WriteAt(i, eVertexAttribute::Bitangent, tangentFrame.bitangents[i]);
                }

                // write texcoord
                if (bHasTexCoord)
                {
                    vertWriter.WriteAt(i, eVertexAttribute::TexCoord0, _texCoordsOrEmpty[i]);
                }
            }

            return Mesh::Load(_positions, attribData, _attribLayoutOrEmpty, _idxData);
        }
    }

}   // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Ref<Mesh> CreateCubeMesh(
    const Vec3          _size,
    const VertexLayout& _attribLayoutOrEmpty)
{
    const float hWidth  = _size.x * 0.5f;
    const float hHeight = _size.y * 0.5f;
    const float hDepth  = _size.z * 0.5f;

    const Vec3 positions[] = {
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

    constexpr Vec2 kTexcoords[] = {
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
    };

    constexpr UInt16 kIndices[] = {
        0,
        1,
        2,
        0,
        2,
        3,
        4,
        5,
        6,
        4,
        6,
        7,
        8,
        9,
        10,
        8,
        10,
        11,
        12,
        13,
        14,
        12,
        14,
        15,
        16,
        17,
        18,
        16,
        18,
        19,
        20,
        21,
        22,
        20,
        22,
        23,
    };

    return CreateMesh_(positions, kTexcoords, _attribLayoutOrEmpty, kIndices, false);
}

Ref<Mesh> CreateSphereMesh(
    const float         _radius,
    const Int32         _divisionLevel,
    const VertexLayout& _attribLayoutOrEmpty)
{
    const float t = (1.0f + Sqrt(5.0f)) / 2.0f;

    const Vec3 positions[] = {
        { -1.f,   +t,  0.f },
        { +1.f,   +t,  0.f },
        { -1.f,   -t,  0.f },
        { +1.f,   -t,  0.f },
        {  0.f, -1.f,   +t },
        {  0.f, +1.f,   +t },
        {  0.f, -1.f,   -t },
        {  0.f, +1.f,   -t },
        {   +t,  0.f, -1.f },
        {   +t,  0.f, +1.f },
        {   -t,  0.f, -1.f },
        {   -t,  0.f, +1.f },
    };

    constexpr Vec2 kTexCoords[] = {
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

    constexpr UInt16 kIndices[] = {
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
        LoopSubdivisionOutputWithTexcoords output = LoopSubdivide(positions, kTexCoords, kIndices, false, _divisionLevel);

        // normalize and scale to sphere radius
        for (Vec3& pos: output.positions)
        {
            pos.Normalize();
            pos *= _radius;
        }

        return CreateMesh_(output.positions, output.texcoords, _attribLayoutOrEmpty, output.indices, output.bUseU32);
    }
    else
    {
        // return base icosahedron
        return CreateMesh_(positions, kTexCoords, _attribLayoutOrEmpty, kIndices, false);
    }
}

Ref<Mesh> CreatePlaneMesh(
    const Vec2&         _size,
    const VertexLayout& _attribLayoutOrEmpty)
{
    const float hWidth = _size.x * 0.5f;
    const float hDepth = _size.y * 0.5f;

    const Vec3 positions[] = {
        { -hWidth, 0.f, -hDepth },
        { -hWidth, 0.f, +hDepth },
        { +hWidth, 0.f, +hDepth },
        { +hWidth, 0.f, -hDepth },
    };

    constexpr Vec2 kTexcoords[] = {
        { 0.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f },
        { 1.f, 1.f },
    };

    constexpr UInt16 kIndices[] = {
        0, 1, 2, 0, 2, 3
    };

    return CreateMesh_(positions, kTexcoords, _attribLayoutOrEmpty, kIndices, false);
}

Ref<Mesh> CreateCircle(
    const float         _radius,
    const Int32         _numTri,
    const VertexLayout& _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTri >= 6, "Circle must have at least 6 triangles.");

    const Int32 numVerts = _numTri + 1;   // triangles + center vertex
    JAM_ASSERT(std::cmp_less_equal(numVerts, Limits<UInt16>::kMax), "Too many triangles for circle.");

    Vector<Vec3> positions(numVerts);
    Vector<Vec2> texcoords(numVerts);

    // center vertex
    positions[0] = Vec3::kZero;
    texcoords[0] = Vec2 { 0.5f, 0.5f };

    // circle vertices
    const float step = k2Pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float s   = Sin(rad);
        const float c   = Cos(rad);
        const float x   = c * _radius;
        const float z   = s * _radius;

        positions[i + 1] = Vec3 { x, 0.f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // generate indices
    Vector<UInt16> idxes(_numTri * 3);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        idxes[i * 3 + 0] = 0;                       // center
        idxes[i * 3 + 1] = (i + 1) % _numTri + 1;   // next vertex
        idxes[i * 3 + 2] = i + 1;                   // current vertex
    }

    return CreateMesh_(positions, texcoords, _attribLayoutOrEmpty, idxes, false);
}

Ref<Mesh> CreateCone(
    const float         _height,
    const float         _radius,
    const Int32         _numTri,
    const VertexLayout& _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTri >= 6, "Cone base must have at least 6 triangles.");

    const Int32 numVerts = _numTri + 2;   // base circle + apex + base center
    JAM_ASSERT(std::cmp_less_equal(numVerts, Limits<UInt16>::kMax), "Too many triangles for cone.");

    Vector<Vec3> position(numVerts);
    Vector<Vec2> texcoords(numVerts);

    // apex vertex
    position[0]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[0] = Vec2 { 0.5f, 0.f };

    // base circle vertices
    const float step = k2Pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        // const auto [s, c] = SinCosApprox(rad);
        const float s = Sin(rad);
        const float c = Cos(rad);
        const float x = c * _radius;
        const float z = s * _radius;

        position[i + 1]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    // base center vertex
    const Int32 bottomCenterIdx = numVerts - 1;
    position[bottomCenterIdx]   = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[bottomCenterIdx]  = Vec2 { 0.5f, 0.5f };

    // generate indices
    Vector<UInt16> idxes(_numTri * 6);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const Int32 nextIdx = (i + 1) % _numTri + 1;
        const Int32 currIdx = i + 1;

        // side triangle
        idxes[i * 6 + 0] = 0;         // apex
        idxes[i * 6 + 1] = nextIdx;   // next base vertex
        idxes[i * 6 + 2] = currIdx;   // current base vertex

        // base triangle
        idxes[i * 6 + 3] = bottomCenterIdx;   // base center
        idxes[i * 6 + 4] = currIdx;           // current base vertex
        idxes[i * 6 + 5] = nextIdx;           // next base vertex
    }

    return CreateMesh_(position, texcoords, _attribLayoutOrEmpty, idxes, false);
}

Ref<Mesh> CreateCylinder(
    const float         _height,
    const float         _radius,
    const Int32         _numTries,
    const VertexLayout& _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTries >= 6, "Cylinder must have at least 6 triangles.");

    const Int32 numVerts = _numTries * 2 + 2;   // top + bottom circles + 2 centers
    JAM_ASSERT(std::cmp_less_equal(numVerts, Limits<UInt16>::kMax), "Too many triangles for cone.");

    Vector<Vec3> position(numVerts);
    Vector<Vec2> texcoords(numVerts);

    // top circle vertices
    const float step = k2Pi / static_cast<float>(_numTries);
    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float s   = Sin(rad);
        const float c   = Cos(rad);
        const float x   = c * _radius;
        const float z   = s * _radius;

        position[i]  = Vec3 { x, _height * 0.5f, z };
        texcoords[i] = Vec2 { static_cast<float>(i) / static_cast<float>(_numTries), 1.f };
    }

    // bottom circle vertices
    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float s   = Sin(rad);
        const float c   = Cos(rad);
        const float x   = c * _radius;
        const float z   = s * _radius;

        position[i + _numTries]  = Vec3 { x, -_height * 0.5f, z };
        texcoords[i + _numTries] = Vec2 { static_cast<float>(i) / static_cast<float>(_numTries), 0.f };
    }

    // center vertices
    const Int32 topCircleCenterIdx = _numTries * 2;
    const Int32 botCircleCenterIdx = _numTries * 2 + 1;

    position[topCircleCenterIdx]  = Vec3 { 0.f, _height * 0.5f, 0.f };
    texcoords[topCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    position[botCircleCenterIdx]  = Vec3 { 0.f, -_height * 0.5f, 0.f };
    texcoords[botCircleCenterIdx] = Vec2 { 0.5f, 0.5f };

    // generate indices
    Vector<UInt16> idxes(_numTries * 12);
    for (Int32 i = 0; i < _numTries; ++i)
    {
        const Int32 nextIdx = (i + 1) % _numTries;
        const Int32 topCurr = i;
        const Int32 topNext = nextIdx;
        const Int32 botCurr = i + _numTries;
        const Int32 botNext = nextIdx + _numTries;

        // side triangles (2 per segment)
        idxes[i * 12 + 0] = topCurr;
        idxes[i * 12 + 1] = topNext;
        idxes[i * 12 + 2] = botCurr;

        idxes[i * 12 + 3] = topNext;
        idxes[i * 12 + 4] = botNext;
        idxes[i * 12 + 5] = botCurr;

        // top heapCap triangle
        idxes[i * 12 + 6] = topCircleCenterIdx;
        idxes[i * 12 + 7] = topNext;
        idxes[i * 12 + 8] = topCurr;

        // bottom heapCap triangle
        idxes[i * 12 + 9]  = botCircleCenterIdx;
        idxes[i * 12 + 10] = botCurr;
        idxes[i * 12 + 11] = botNext;
    }

    return CreateMesh_(position, texcoords, _attribLayoutOrEmpty, idxes, false);
}

Vector<Vec3> ComputeNormals(
    const Span<const Vec3> _positions,
    const MemoryView       _idxData,
    const bool             _bUseU32)
{
    IndexReader indexReader { _idxData, _bUseU32 };
    const Int32 numVerts = static_cast<Int32>(_positions.size());
    const Int32 numIdxes = CalcNumIndices(_idxData, _bUseU32);
    const Int32 numFaces = numIdxes / 3;

    // accumulate weighted normals per vertex
    Vector<Vec3> tmpNormals(numVerts);
    for (Int32 i = 0; i < numFaces; ++i)
    {
        // read triangle indices
        const UInt32 i0 = indexReader.ReadAt<UInt32>(i * 3 + 0);
        const UInt32 i1 = indexReader.ReadAt<UInt32>(i * 3 + 1);
        const UInt32 i2 = indexReader.ReadAt<UInt32>(i * 3 + 2);
        JAM_ASSERT(std::cmp_less(i0, numVerts) && std::cmp_less(i1, numVerts) && std::cmp_less(i2, numVerts), "Index out of range.");

        // get triangle positions
        const Vec3 p0 = _positions[i0];
        const Vec3 p1 = _positions[i1];
        const Vec3 p2 = _positions[i2];

        const Vec3 p01 = (p1 - p0).Normalized();
        const Vec3 p02 = (p2 - p0).Normalized();
        const Vec3 p12 = (p2 - p1).Normalized();

        // degenerate triangle check
        if (p01.IsZeroApprox() || p02.IsZeroApprox() || p12.IsZeroApprox())
        {
            continue;
        }

        // calculate face normal
        const Vec3 faceNormal = p01.Cross(p02).Normalized();

        // calculate angle weights for each vertexs
        const float angle0 = Acos(Clamp(p01.Dot(p02), -1.0f, 1.0f));
        const float angle1 = Acos(Clamp(p12.Dot(-p01), -1.0f, 1.0f));
        const float angle2 = Acos(Clamp(-p02.Dot(-p12), -1.0f, 1.0f));

        // accumulate angle-weighted face normals
        tmpNormals[i0] += faceNormal * angle0;
        tmpNormals[i1] += faceNormal * angle1;
        tmpNormals[i2] += faceNormal * angle2;
    }

    // normalize accumulated normals
    Vector<Vec3> normals = {};
    normals.reserve(numVerts);
    for (SizeT i = 0; i < numVerts; ++i)
    {
        normals.emplace_back(tmpNormals[i].Normalized());
    }

    return normals;
}

TangentFrame ComputeTangentFrame(
    const Span<const Vec3> _positions,
    const Span<const Vec3> _normals,
    const Span<const Vec2> _texCoords,
    const MemoryView       _idxData,
    const bool             _bUseU32)
{
    IndexReader indexReader { _idxData, _bUseU32 };
    const Int32 numVerts = static_cast<Int32>(_positions.size());
    const Int32 numIdxes = CalcNumIndices(_idxData, _bUseU32);
    const Int32 numFaces = numIdxes / 3;

    // temporary accumulation vectors
    Vector<Vec3> tan1(numVerts);
    Vector<Vec3> tan2(numVerts);
    for (Int32 i = 0; i < numFaces; ++i)
    {
        // read triangle indices
        const UInt32 i0 = indexReader.ReadAt<UInt32>(i * 3 + 0);
        const UInt32 i1 = indexReader.ReadAt<UInt32>(i * 3 + 1);
        const UInt32 i2 = indexReader.ReadAt<UInt32>(i * 3 + 2);

        JAM_ASSERT(std::cmp_less(i0, numVerts) && std::cmp_less(i1, numVerts) && std::cmp_less(i2, numVerts), "Index out of range.");

        // get triangle data
        const Vec3 v0 = _positions[i0];
        const Vec3 v1 = _positions[i1];
        const Vec3 v2 = _positions[i2];

        const Vec2 w0 = _texCoords[i0];
        const Vec2 w1 = _texCoords[i1];
        const Vec2 w2 = _texCoords[i2];

        // calculate position deltas
        const float x1 = v1.x - v0.x;
        const float x2 = v2.x - v0.x;
        const float y1 = v1.y - v0.y;
        const float y2 = v2.y - v0.y;
        const float z1 = v1.z - v0.z;
        const float z2 = v2.z - v0.z;
        if (IsZeroApprox(x1) && IsZeroApprox(y1) && IsZeroApprox(z1)
            || IsZeroApprox(x2) && IsZeroApprox(y2) && IsZeroApprox(z2))
        {
            continue;   // degenerate triangle
        }

        // calculate uv deltas
        const float s1 = w1.x - w0.x;
        const float s2 = w2.x - w0.x;
        const float t1 = w1.y - w0.y;
        const float t2 = w2.y - w0.y;
        if (IsZeroApprox(s1) && IsZeroApprox(t1)
            || IsZeroApprox(s2) && IsZeroApprox(t2))
        {
            continue;   // degenerate uv mapping
        }

        // calculate determinant and inverse
        float det = s1 * t2 - s2 * t1;
        det       = IsZeroApprox(det) ? 1.0f : 1.0f / det;

        // calculate tangent and bitangent directions
        const Vec3 sdir = {
            (t2 * x1 - t1 * x2) * det,
            (t2 * y1 - t1 * y2) * det,
            (t2 * z1 - t1 * z2) * det
        };

        const Vec3 tdir = {
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
    Vector<Vec3> tans = {};
    tans.reserve(numVerts);

    Vector<Vec3> bitans = {};
    bitans.reserve(numVerts);

    for (SizeT i = 0; i < numVerts; ++i)
    {
        const Vec3 n = _normals[i];
        const Vec3 t = tan1[i];

        // orthogonalize
        Vec3 tan = (t - n * n.Dot(t)).Normalized();

        // handle degenerate case
        if (tan.IsZeroApprox())
        {
            // choose an arbitrary perpendicular axis
            Vec3 axis;
            if (n.x < n.y && n.x < n.z)
            {
                axis = Vec3 { 1.f, 0.f, 0.f };   // x axis
            }
            else if (n.y < n.z)
            {
                axis = Vec3 { 0.f, 1.f, 0.f };   // x axis
            }
            else
            {
                axis = Vec3 { 0.f, 0.f, 1.f };   // x axis
            }

            tan = n.Cross(axis).Normalized();
        }

        // calculate bitangent and determine handedness
        Vec3        bitan      = n.Cross(tan).Normalized();
        const float handedness = n.Cross(tan).Dot(tan2[i]);
        if (handedness < 0.0f)
        {
            bitan = -bitan;
        }

        tans.emplace_back(tan);
        bitans.emplace_back(bitan);
    }

    TangentFrame output = {};
    output.tangents     = std::move(tans);
    output.bitangents   = std::move(bitans);
    return output;
}

}   // namespace Jam
