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
        const MemoryView       _indexData,
        const bool             _bUseU32)
    {
        JAM_ASSERT(!_positions.empty(), "Position data is empty.");
        JAM_ASSERT(!_indexData.IsEmpty(), "Index data is empty.");
        JAM_ASSERT(!_attribLayoutOrEmpty.Has(eVertexAttribute::Position), "Position attribute should not be included in the vertex layout, as it is provided separately.");

        const bool bPosOnly      = _attribLayoutOrEmpty.IsEmpty();
        const bool bHasTexCoord  = _attribLayoutOrEmpty.Has(eVertexAttribute::TexCoord0);
        const bool bHasNormal    = _attribLayoutOrEmpty.Has(eVertexAttribute::Normal);
        const bool bHasTangent   = _attribLayoutOrEmpty.Has(eVertexAttribute::Tangent);
        const bool bHasBitangent = _attribLayoutOrEmpty.Has(eVertexAttribute::Bitangent);

        Vector<Vec3> normals = {};
        if (bHasNormal)
        {
            normals = ComputeNormals(_positions, _indexData, _bUseU32);
        }

        TangentFrame tangentFrame = {};
        if (bHasTangent || bHasBitangent)
        {
            JAM_ASSERT(bHasTexCoord, "TexCoord0 attribute is required to compute tangents and bitangents.");
            JAM_ASSERT(!_texCoordsOrEmpty.empty(), "TexCoord0 data is required to compute tangents and bitangents.");
            JAM_ASSERT(!normals.empty(), "Normals must be computed before computing tangents and bitangents.");
            tangentFrame = ComputeTangentFrame(_positions, normals, _texCoordsOrEmpty, _indexData, _bUseU32);
        }

        if (bPosOnly)
        {
            return Mesh::Load(_positions, _indexData);
        }
        else
        {
            const SizeT  numVerts = _positions.size();
            Vector<Byte> attribData(_attribLayoutOrEmpty.GetStride() * numVerts);

            VertexWriter vertWriter { attribData, _attribLayoutOrEmpty };
            for (SizeT i = 0; i < numVerts; ++i)
            {
                if (bHasNormal)
                    vertWriter.WriteAt(i, eVertexAttribute::Normal, normals[i]);

                if (bHasTangent)
                    vertWriter.WriteAt(i, eVertexAttribute::Tangent, tangentFrame.tangents[i]);

                if (bHasBitangent)
                    vertWriter.WriteAt(i, eVertexAttribute::Bitangent, tangentFrame.bitangents[i]);

                if (bHasTexCoord)
                    vertWriter.WriteAt(i, eVertexAttribute::TexCoord0, _texCoordsOrEmpty[i]);
            }

            return Mesh::Load(_positions, attribData, _attribLayoutOrEmpty, _indexData);
        }
    }

    [[nodiscard]] Quat NormalToRotation(const Vec3 _normal) noexcept
    {
        const Vec3 from = Vec3::kUp;   // { 0, 1, 0 }
        const Vec3 to   = _normal.Normalized();

        const float dot = from.Dot(to);

        if (dot >= 1.f - kEpsilon)
            return Quat::kIdentity;

        if (dot <= -1.f + kEpsilon)
        {
            // 180도 회전 — 임의의 수직 축 사용
            Vec3 axis = Vec3::kRight.Cross(from);
            if (axis.IsZeroApprox())
                axis = Vec3::kForward.Cross(from);
            return Quat::FromAxisAngle(axis.Normalized(), kPi);
        }

        const Vec3  axis  = from.Cross(to).Normalized();
        const float angle = Acos(Clamp(dot, -1.f, 1.f));
        return Quat::FromAxisAngle(axis, angle);
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
        // front (-Z)
        { -hWidth, -hHeight, -hDepth },
        { -hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { +hWidth, -hHeight, -hDepth },
        // back (+Z)
        { +hWidth, -hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        { -hWidth, -hHeight, +hDepth },
        // left (-X)
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, +hHeight, +hDepth },
        { -hWidth, +hHeight, -hDepth },
        { -hWidth, -hHeight, -hDepth },
        // right (+X)
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, +hHeight, -hDepth },
        { +hWidth, +hHeight, +hDepth },
        { +hWidth, -hHeight, +hDepth },
        // top (+Y)
        { -hWidth, +hHeight, -hDepth },
        { -hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, +hDepth },
        { +hWidth, +hHeight, -hDepth },
        // bottom (-Y)
        { -hWidth, -hHeight, +hDepth },
        { -hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, -hDepth },
        { +hWidth, -hHeight, +hDepth },
    };

    constexpr Vec2 kTexcoords[] = {
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

    constexpr UInt16 kIndices[] = {
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

    return CreateMesh_(positions, kTexcoords, _attribLayoutOrEmpty, kIndices, false);
}

Ref<Mesh> CreateSphereMesh(
    const float         _radius,
    const Int32         _divisionLevel,
    const VertexLayout& _attribLayoutOrEmpty)
{
    const float t = (1.0f + Sqrt(5.0f)) / 2.0f;

    Vec3 positions[] = {
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
        LoopSubdivisionOutputWithTexcoords output = LoopSubdivide(positions, kTexCoords, kIndices, false, _divisionLevel);

        for (Vec3& pos: output.positions)
        {
            pos.Normalize();
            pos *= _radius;
        }

        return CreateMesh_(output.positions, output.texcoords, _attribLayoutOrEmpty, output.indices, output.bUseU32);
    }
    else
    {
        for (Vec3& pos: positions)
        {
            pos.Normalize();
            pos *= _radius;
        }

        return CreateMesh_(positions, kTexCoords, _attribLayoutOrEmpty, kIndices, false);
    }
}

Ref<Mesh> CreatePlaneMesh(
    const Vec2          _size,
    const Vec2          _uv0,
    const Vec2          _uv1,
    const Vec3          _normal,
    const VertexLayout& _attribLayoutOrEmpty)
{
    const float hWidth = _size.x * 0.5f;
    const float hDepth = _size.y * 0.5f;

    Vector<Vec3> positions = {
        { -hWidth, 0.f, -hDepth },
        { -hWidth, 0.f, +hDepth },
        { +hWidth, 0.f, +hDepth },
        { +hWidth, 0.f, -hDepth },
    };

    const Vec2 kTexcoords[] = {
        { _uv0.x, _uv1.y },
        { _uv0.x, _uv0.y },
        { _uv1.x, _uv0.y },
        { _uv1.x, _uv1.y },
    };

    constexpr UInt16 kIndices[] = {
        0, 1, 2, 0, 2, 3
    };

    if (!_normal.IsEqualApprox(Vec3::kUp))
    {
        const Quat q = Quat::FromTo(Vec3::kUp, _normal.Normalized());
        for (Vec3& pos: positions)
        {
            pos = q.Rotate(pos);
        }
    }

    return CreateMesh_(positions, kTexcoords, _attribLayoutOrEmpty, kIndices, false);
}

Ref<Mesh> CreatePlaneMesh(
    const Vec2          _size,
    const Vec3          _normal,
    const VertexLayout& _attribLayoutOrEmpty)
{
    return CreatePlaneMesh(_size, { 0.f, 0.f }, { 1.f, 1.f }, _normal, _attribLayoutOrEmpty);
}

Ref<Mesh> CreateCircleMesh(
    const float         _radius,
    const Vec3          _normal,
    const Int32         _numTri,
    const VertexLayout& _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTri >= 6, "Circle must have at least 6 triangles.");

    const Int32 numVerts = _numTri + 1;
    JAM_ASSERT(LessEqual(numVerts, Limits<UInt16>::kMax), "Too many triangles for circle.");

    Vector<Vec3> positions(numVerts);
    Vector<Vec2> texcoords(numVerts);

    positions[0] = Vec3::kZero;
    texcoords[0] = Vec2 { 0.5f, 0.5f };

    const float step = k2Pi / static_cast<float>(_numTri);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);
        const float x   = c * _radius;
        const float z   = s * _radius;

        positions[i + 1] = Vec3 { x, 0.f, z };
        texcoords[i + 1] = Vec2 { (x / _radius + 1.f) * 0.5f, (z / _radius + 1.f) * 0.5f };
    }

    Vector<UInt16> indices(_numTri * 3);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = (i + 1) % _numTri + 1;
        indices[i * 3 + 2] = i + 1;
    }

    if (!_normal.IsEqualApprox(Vec3::kUp))
    {
        const Quat q = Quat::FromTo(Vec3::kUp, _normal.Normalized());
        for (Vec3& pos: positions)
        {
            pos = q.Rotate(pos);
        }
    }

    return CreateMesh_(positions, texcoords, _attribLayoutOrEmpty, indices, false);
}
Ref<Mesh> CreateConeMesh(
    const float           _height,
    const float           _radius,
    const Vec3            _normal,
    const Int32           _numTri,
    const eCreateConeMesh _option,
    const VertexLayout&   _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTri >= 6, "Cone base must have at least 6 triangles.");

    const Int32 numVerts = _numTri * 2 + 2;
    JAM_ASSERT(LessEqual(numVerts, Limits<UInt16>::kMax), "Too many triangles for cone.");

    Vector<Vec3> position(numVerts);
    Vector<Vec2> texcoords(numVerts);

    float originOffsetY = 0.f;
    switch (_option)
    {
        case eCreateConeMesh::OriginAtCenter: originOffsetY = 0.f; break;
        case eCreateConeMesh::OriginAtBase: originOffsetY = _height * 0.5f; break;
        case eCreateConeMesh::OriginAtApex: originOffsetY = -_height * 0.5f; break;
    }

    const float apexY = _height * 0.5f + originOffsetY;
    const float baseY = -_height * 0.5f + originOffsetY;
    const float step  = k2Pi / static_cast<float>(_numTri);

    position[0]  = Vec3 { 0.f, apexY, 0.f };
    texcoords[0] = Vec2 { 0.5f, 0.f };

    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        position[i + 1]  = Vec3 { c * _radius, baseY, s * _radius };
        texcoords[i + 1] = Vec2 { (c + 1.f) * 0.5f, 1.f };
    }

    for (Int32 i = 0; i < _numTri; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        const Int32 index = i + _numTri + 1;
        position[index]   = Vec3 { c * _radius, baseY, s * _radius };
        texcoords[index]  = Vec2 { (c + 1.f) * 0.5f, (s + 1.f) * 0.5f };
    }

    const Int32 baseCenterIdx = _numTri * 2 + 1;
    position[baseCenterIdx]   = Vec3 { 0.f, baseY, 0.f };
    texcoords[baseCenterIdx]  = Vec2 { 0.5f, 0.5f };

    Vector<UInt16> indices(_numTri * 6);
    for (Int32 i = 0; i < _numTri; ++i)
    {
        const Int32 sideNext = (i + 1) % _numTri + 1;
        const Int32 sideCurr = i + 1;
        const Int32 baseNext = (i + 1) % _numTri + _numTri + 1;
        const Int32 baseCurr = i + _numTri + 1;

        indices[i * 6 + 0] = 0;
        indices[i * 6 + 1] = sideNext;
        indices[i * 6 + 2] = sideCurr;

        indices[i * 6 + 3] = baseCenterIdx;
        indices[i * 6 + 4] = baseCurr;
        indices[i * 6 + 5] = baseNext;
    }

    if (!_normal.IsEqualApprox(Vec3::kUp))
    {
        const Quat q = Quat::FromTo(Vec3::kUp, _normal.Normalized());
        for (Vec3& pos: position)
        {
            pos = q.Rotate(pos);
        }
    }

    return CreateMesh_(position, texcoords, _attribLayoutOrEmpty, indices, false);
}

Ref<Mesh> CreateCylinderMesh(
    const float               _height,
    const float               _radius,
    const Vec3                _normal,
    const Int32               _numTries,
    const eCreateCylinderMesh _option,
    const VertexLayout&       _attribLayoutOrEmpty)
{
    JAM_ASSERT(_numTries >= 6, "Cylinder must have at least 6 triangles.");

    const Int32 numVerts = _numTries * 4 + 2;
    JAM_ASSERT(LessEqual(numVerts, Limits<UInt16>::kMax), "Too many triangles for cylinder.");

    Vector<Vec3> position(numVerts);
    Vector<Vec2> texcoords(numVerts);

    float originOffsetY = 0.f;
    switch (_option)
    {
        case eCreateCylinderMesh::OriginAtCenter: originOffsetY = 0.f; break;
        case eCreateCylinderMesh::OriginAtBase: originOffsetY = _height * 0.5f; break;
        case eCreateCylinderMesh::OriginAtTop: originOffsetY = -_height * 0.5f; break;
    }

    const float step  = k2Pi / static_cast<float>(_numTries);
    const float halfH = _height * 0.5f;
    const float topY  = +halfH + originOffsetY;
    const float botY  = -halfH + originOffsetY;

    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        position[i]  = Vec3 { c * _radius, topY, s * _radius };
        texcoords[i] = Vec2 { (c + 1.f) * 0.5f, (s + 1.f) * 0.5f };
    }

    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        const Int32 index = i + _numTries;
        position[index]   = Vec3 { c * _radius, botY, s * _radius };
        texcoords[index]  = Vec2 { (c + 1.f) * 0.5f, (s + 1.f) * 0.5f };
    }

    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        const Int32 index = i + _numTries * 2;
        position[index]   = Vec3 { c * _radius, topY, s * _radius };
        texcoords[index]  = Vec2 { static_cast<float>(i) / static_cast<float>(_numTries), 1.f };
    }

    for (Int32 i = 0; i < _numTries; ++i)
    {
        const float rad = static_cast<float>(i) * step;
        const float c   = Cos(rad);
        const float s   = Sin(rad);

        const Int32 index = i + _numTries * 3;
        position[index]   = Vec3 { c * _radius, botY, s * _radius };
        texcoords[index]  = Vec2 { static_cast<float>(i) / static_cast<float>(_numTries), 0.f };
    }

    // top cap center
    const Int32 topCenterIdx = _numTries * 4;
    position[topCenterIdx]   = Vec3 { 0.f, topY, 0.f };
    texcoords[topCenterIdx]  = Vec2 { 0.5f, 0.5f };

    // bottom cap center
    const Int32 botCenterIdx = _numTries * 4 + 1;
    position[botCenterIdx]   = Vec3 { 0.f, botY, 0.f };
    texcoords[botCenterIdx]  = Vec2 { 0.5f, 0.5f };

    Vector<UInt16> indices(_numTries * 12);

    const Int32 sideTopBase = _numTries * 2;
    const Int32 sideBotBase = _numTries * 3;

    for (Int32 i = 0; i < _numTries; ++i)
    {
        const Int32 next = (i + 1) % _numTries;

        const Int32 stCurr = sideTopBase + i;
        const Int32 stNext = sideTopBase + next;
        const Int32 sbCurr = sideBotBase + i;
        const Int32 sbNext = sideBotBase + next;

        const Int32 tcCurr = i;
        const Int32 tcNext = next;
        const Int32 bcCurr = i + _numTries;
        const Int32 bcNext = next + _numTries;

        indices[i * 12 + 0] = stCurr;
        indices[i * 12 + 1] = stNext;
        indices[i * 12 + 2] = sbCurr;

        indices[i * 12 + 3] = stNext;
        indices[i * 12 + 4] = sbNext;
        indices[i * 12 + 5] = sbCurr;

        indices[i * 12 + 6] = topCenterIdx;
        indices[i * 12 + 7] = tcNext;
        indices[i * 12 + 8] = tcCurr;

        indices[i * 12 + 9]  = botCenterIdx;
        indices[i * 12 + 10] = bcCurr;
        indices[i * 12 + 11] = bcNext;
    }

    if (!_normal.IsEqualApprox(Vec3::kUp))
    {
        const Quat q = Quat::FromTo(Vec3::kUp, _normal.Normalized());
        for (Vec3& pos: position)
        {
            pos = q.Rotate(pos);
        }
    }

    return CreateMesh_(position, texcoords, _attribLayoutOrEmpty, indices, false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

Vector<Vec3> ComputeNormals(
    const Span<const Vec3> _positions,
    const MemoryView       _indexData,
    const bool             _bUseU32)
{
    IndexReader indexReader { _indexData, _bUseU32 };
    const Int32 numVerts = static_cast<Int32>(_positions.size());
    const Int32 numIdxes = CalcNumIndices(_indexData, _bUseU32);
    const Int32 numFaces = numIdxes / 3;

    Vector<Vec3> tmpNormals(numVerts);
    for (Int32 i = 0; i < numFaces; ++i)
    {
        const UInt32 i0 = indexReader.ReadAt<UInt32>(i * 3 + 0);
        const UInt32 i1 = indexReader.ReadAt<UInt32>(i * 3 + 1);
        const UInt32 i2 = indexReader.ReadAt<UInt32>(i * 3 + 2);
        JAM_ASSERT(Less(i0, numVerts) && Less(i1, numVerts) && Less(i2, numVerts), "Index out of range.");

        const Vec3 p0 = _positions[i0];
        const Vec3 p1 = _positions[i1];
        const Vec3 p2 = _positions[i2];

        const Vec3 p01 = (p1 - p0).Normalized();
        const Vec3 p02 = (p2 - p0).Normalized();
        const Vec3 p12 = (p2 - p1).Normalized();

        if (p01.IsZeroApprox() || p02.IsZeroApprox() || p12.IsZeroApprox())
        {
            continue;
        }

        const Vec3 faceNormal = p01.Cross(p02).Normalized();

        const float angle0 = Acos(Clamp(p01.Dot(p02), -1.0f, 1.0f));
        const float angle1 = Acos(Clamp(p12.Dot(-p01), -1.0f, 1.0f));
        const float angle2 = Acos(Clamp(-p02.Dot(-p12), -1.0f, 1.0f));

        tmpNormals[i0] += faceNormal * angle0;
        tmpNormals[i1] += faceNormal * angle1;
        tmpNormals[i2] += faceNormal * angle2;
    }

    Vector<Vec3> normals = {};
    normals.reserve(numVerts);
    for (SizeT i = 0; i < numVerts; ++i)
        normals.emplace_back(tmpNormals[i].Normalized());

    return normals;
}

TangentFrame ComputeTangentFrame(
    const Span<const Vec3> _positions,
    const Span<const Vec3> _normals,
    const Span<const Vec2> _texCoords,
    const MemoryView       _indexData,
    const bool             _bUseU32)
{
    IndexReader indexReader { _indexData, _bUseU32 };
    const Int32 numVerts = static_cast<Int32>(_positions.size());
    const Int32 numIdxes = CalcNumIndices(_indexData, _bUseU32);
    const Int32 numFaces = numIdxes / 3;

    Vector<Vec3> tan1(numVerts);
    Vector<Vec3> tan2(numVerts);
    for (Int32 i = 0; i < numFaces; ++i)
    {
        const UInt32 i0 = indexReader.ReadAt<UInt32>(i * 3 + 0);
        const UInt32 i1 = indexReader.ReadAt<UInt32>(i * 3 + 1);
        const UInt32 i2 = indexReader.ReadAt<UInt32>(i * 3 + 2);
        JAM_ASSERT(Less(i0, numVerts) && Less(i1, numVerts) && Less(i2, numVerts), "Index out of range.");

        const Vec3 v0 = _positions[i0];
        const Vec3 v1 = _positions[i1];
        const Vec3 v2 = _positions[i2];

        const Vec2 w0 = _texCoords[i0];
        const Vec2 w1 = _texCoords[i1];
        const Vec2 w2 = _texCoords[i2];

        const float x1 = v1.x - v0.x;
        const float x2 = v2.x - v0.x;
        const float y1 = v1.y - v0.y;
        const float y2 = v2.y - v0.y;
        const float z1 = v1.z - v0.z;
        const float z2 = v2.z - v0.z;
        if (IsZeroApprox(x1) && IsZeroApprox(y1) && IsZeroApprox(z1) || IsZeroApprox(x2) && IsZeroApprox(y2) && IsZeroApprox(z2))
        {
            continue;
        }

        const float s1 = w1.x - w0.x;
        const float s2 = w2.x - w0.x;
        const float t1 = w1.y - w0.y;
        const float t2 = w2.y - w0.y;
        if (IsZeroApprox(s1) && IsZeroApprox(t1) || IsZeroApprox(s2) && IsZeroApprox(t2))
        {
            continue;
        }

        float det = s1 * t2 - s2 * t1;
        det       = IsZeroApprox(det) ? 1.0f : 1.0f / det;

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

        tan1[i0] += sdir;
        tan1[i1] += sdir;
        tan1[i2] += sdir;

        tan2[i0] += tdir;
        tan2[i1] += tdir;
        tan2[i2] += tdir;
    }

    Vector<Vec3> tans = {};
    tans.reserve(numVerts);

    Vector<Vec3> bitans = {};
    bitans.reserve(numVerts);

    for (SizeT i = 0; i < numVerts; ++i)
    {
        const Vec3 n = _normals[i];
        const Vec3 t = tan1[i];

        Vec3 tan = (t - n * n.Dot(t)).Normalized();

        if (tan.IsZeroApprox())
        {
            Vec3 axis = Vec3::kZero;
            if (n.x < n.y && n.x < n.z)
            {
                axis = Vec3 { 1.f, 0.f, 0.f };
            }
            else if (n.y < n.z)
            {
                axis = Vec3 { 0.f, 1.f, 0.f };
            }
            else
            {
                axis = Vec3 { 0.f, 0.f, 1.f };
            }

            tan = n.Cross(axis).Normalized();
        }

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