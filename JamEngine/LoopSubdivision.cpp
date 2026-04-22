#include "pch.h"

#include "LoopSubdivision.h"

#include "GeometryUtilities.h"
#include "MemoryArena.h"
#include "MemoryHasher.h"

// 루프 세분화 알고리즘
// https://pbr-book.org/3ed-2018/Shapes/Subdivision_Surfaces
// https://en.wikipedia.org/wiki/Loop_subdivision_surface

namespace Jam
{

namespace
{

    struct SDFace;
    struct SDVertex;

    struct SDPoint
    {
        [[nodiscard]] SDPoint operator*(const float _scalar) const
        {
            SDPoint p  = {};
            p.position = position * _scalar;
            p.texcoord = texcoord * _scalar;
            return p;
        }

        [[nodiscard]] SDPoint operator+(const SDPoint& _other) const
        {
            SDPoint p  = {};
            p.position = position + _other.position;
            p.texcoord = texcoord + _other.texcoord;
            return p;
        }

        SDPoint& operator+=(const SDPoint& _other)
        {
            position += _other.position;
            texcoord += _other.texcoord;
            return *this;
        }

        Vec3 position = Vec3::kZero;
        Vec2 texcoord = Vec2::kZero;
    };

    struct SDVertex
    {
        [[nodiscard]] Int32           CalcValence() const;
        [[nodiscard]] Vector<SDPoint> OneRing() const;

        SDPoint   point      = {};        // 정점의 위치 및 텍스처 좌표
        SDFace*   pStartFace = nullptr;   // 임의의 면
        SDVertex* pChildVert = nullptr;   // 세분화 후 생성된 자식 버텍스
        bool      bRegular   = false;     // 정점이 Regular한지 여부
        bool      bBoundary  = false;     // 경계 버텍스 여부
    };

    struct SDFace
    {
        [[nodiscard]] Int32 CalcVertexIndex(
            const SDVertex* pVert) const
        {
            for (Int32 i = 0; i < 3; ++i)
            {
                if (pVerts[i] == pVert)
                {
                    return i;
                }
            }
            JAM_UNREACHABLE;
        }

        [[nodiscard]] SDFace* GetNextFace(
            const SDVertex* pVert) const
        {
            return pFaces[CalcVertexIndex(pVert)];
        }

        [[nodiscard]] SDFace* GetPrevFace(
            const SDVertex* pVert) const
        {
            return pFaces[(CalcVertexIndex(pVert) + 2) % 3];
        }

        [[nodiscard]] SDVertex* GetNextVertex(
            const SDVertex* pVert) const
        {
            return pVerts[(CalcVertexIndex(pVert) + 1) % 3];
        }

        [[nodiscard]] SDVertex* GetPrevVertex(
            const SDVertex* pVert) const
        {
            return pVerts[(CalcVertexIndex(pVert) + 2) % 3];
        }

        [[nodiscard]] SDVertex* GetOtherVertex(
            const SDVertex* pV0,
            const SDVertex* pV1) const
        {
            for (Int32 i = 0; i < 3; ++i)
            {
                if (pVerts[i] != pV0 && pVerts[i] != pV1)
                {
                    return pVerts[i];
                }
            }
            JAM_UNREACHABLE;
        }

        SDVertex* pVerts[3]      = { nullptr, nullptr, nullptr };
        SDFace*   pFaces[3]      = { nullptr, nullptr, nullptr };
        SDFace*   pChildFaces[4] = { nullptr, nullptr, nullptr, nullptr };
    };

    struct SDEdge
    {
        SDVertex* pVerts[2]    = { nullptr, nullptr };
        SDFace*   pFaces[2]    = { nullptr, nullptr };
        Int32     face0EdgeNum = -1;
    };

    struct SDEdgeHash
    {
        [[nodiscard]] size_t operator()(const SDEdge& _edge) const noexcept
        {
            Murmur3Hasher64 hasher {};
            hasher.Mix(_edge.pVerts);
            return hasher.Finish();
        }
    };

    struct SDEdgeEqual
    {
        [[nodiscard]] bool operator()(const SDEdge& _lhs, const SDEdge& _rhs) const
        {
            return _lhs.pVerts[0] == _rhs.pVerts[0] && _lhs.pVerts[1] == _rhs.pVerts[1];
        }
    };

    Int32 SDVertex::CalcValence() const
    {
        const SDFace* pFace = pStartFace;
        if (!bBoundary)
        {
            Int32 nf = 1;
            while ((pFace = pFace->GetNextFace(this)) != pStartFace)
            {
                ++nf;
            }

            return nf;
        }
        else
        {
            Int32 nf = 1;
            while ((pFace = pFace->GetNextFace(this)))
            {
                ++nf;
            }

            pFace = pStartFace;
            while ((pFace = pFace->GetPrevFace(this)))
            {
                ++nf;
            }

            return nf + 1;
        }
    }

    Vector<SDPoint> SDVertex::OneRing() const
    {
        const Int32     valence = CalcValence();
        Vector<SDPoint> pRing   = {};
        pRing.reserve(valence);

        if (!bBoundary)
        {
            const SDFace* face = pStartFace;
            do
            {
                pRing.emplace_back(face->GetNextVertex(this)->point);
                face = face->GetNextFace(this);
            } while (face != pStartFace);
        }
        else
        {
            const SDFace* pFace = pStartFace;
            SDFace*       f2;
            while ((f2 = pFace->GetNextFace(this)))
            {
                pFace = f2;
            }

            pRing.emplace_back(pFace->GetNextVertex(this)->point);
            do
            {
                pRing.emplace_back(pFace->GetPrevVertex(this)->point);
                pFace = pFace->GetPrevFace(this);
            } while (pFace);
        }
        return pRing;
    }

    float Beta(
        const Int32 _valence)
    {
        return _valence == 3 ? 3.f / 16.f : 3.f / (8.f * static_cast<float>(_valence));
    }

    float LoopGamma(
        const Int32 _valence)
    {
        return 1.f / (static_cast<float>(_valence) + 3.f / (8.f * Beta(_valence)));
    }

    SDPoint WeightOneRing(
        const SDVertex* _pVert,
        const float     _beta)
    {
        const Int32 valence = _pVert->CalcValence();
        const auto  ring    = _pVert->OneRing();
        const float scale   = 1.0f - static_cast<float>(valence) * _beta;

        SDPoint point = _pVert->point * scale;
        for (const SDPoint& p: ring)
        {
            point += p * _beta;
        }
        return point;
    }

    SDPoint WeightBoundary(
        const SDVertex* _pVert,
        const float     _beta)
    {
        const Int32           valence = _pVert->CalcValence();
        const Vector<SDPoint> ring    = _pVert->OneRing();

        const float scale = 1.0f - 2.0f * _beta;
        SDPoint     point = _pVert->point * scale;

        point += ring[0] * _beta;
        point += ring[valence - 1] * _beta;
        return point;
    }

}   // namespace

////////////////////////////////////////////////////////////////////////////////////////

LoopSubdivisionOutput LoopSubdivide(
    const Span<const Vec3> _positions,
    const MemoryView       _idxData,
    const bool             _bUseU32,
    const Int32            _level)
{
    LoopSubdivisionOutputWithTexcoords result = LoopSubdivide(_positions, {}, _idxData, _bUseU32, _level);

    LoopSubdivisionOutput output = {};
    output.positions             = std::move(result.positions);
    output.indices               = std::move(result.indices);
    output.bUseU32               = result.bUseU32;
    return output;
}

LoopSubdivisionOutputWithTexcoords LoopSubdivide(
    Span<const Vec3> _positions,
    Span<const Vec2> _texcoordsOrEmpty,   // or empty
    MemoryView       _idxData,
    bool             _bUseU32,
    Int32            _level)
{
    JAM_ASSERT(!_positions.empty(), "position data is empty.");
    JAM_ASSERT(_level > 0, "Subdivision level must be greater than 0.");

    MemoryArena arena { 1024 };   // 메모리 아레나

    const IndexReader indexReader { _idxData, _bUseU32 };
    const Int32       numVerts     = static_cast<Int32>(_positions.size());
    const Int32       numIdxes     = CalcNumIndices(_idxData, _bUseU32);
    const Int32       numFaces     = numIdxes / 3;
    const bool        bHasTexCoord = !_texcoordsOrEmpty.empty();

    // 버텍스 및 페이스
    Vector<SDVertex*> verts = {};
    verts.reserve(numVerts);

    Vector<SDFace*> faces = {};
    faces.reserve(numFaces);

    // 초기 데이터 삽입
    for (Int32 i = 0; i < numVerts; ++i)
    {
        SDVertex* pVert = arena.New<SDVertex>();

        pVert->point.position = _positions[i];
        if (bHasTexCoord)
        {
            pVert->point.texcoord = _texcoordsOrEmpty[i];
        }

        verts.emplace_back(pVert);
    }

    // 초기 데이터 삽입
    for (Int32 i = 0; i < numFaces; ++i)
    {
        faces.emplace_back(arena.New<SDFace>());
    }

    // 페이스의 버텍스 정보 설정
    for (Int32 i = 0; i < numFaces; ++i)
    {
        SDFace* pFace = faces[i];
        for (Int32 j = 0; j < 3; ++j)
        {
            const UInt32 idx   = indexReader.ReadAt<UInt32>(i * 3 + j);
            SDVertex*    pVert = verts[idx];
            pFace->pVerts[j]   = pVert;
            pVert->pStartFace  = pFace;   // 임의의 면 설정
        }
    }

    // 엣지 정보 설정
    HashSet<SDEdge, SDEdgeHash, SDEdgeEqual> edgeSet = {};
    for (Int32 i = 0; i < numFaces; ++i)
    {
        SDFace* pFace = faces[i];
        for (Int32 j = 0; j < 3; ++j)
        {
            const Int32 v0 = j;
            const Int32 v1 = (j + 1) % 3;

            SDEdge edge    = {};
            edge.pVerts[0] = Min(pFace->pVerts[v0], pFace->pVerts[v1]);
            edge.pVerts[1] = Max(pFace->pVerts[v0], pFace->pVerts[v1]);

            const auto it = edgeSet.find(edge);
            if (it == edgeSet.end())
            {
                // 새로운 엣지
                edge.pFaces[0]    = pFace;
                edge.face0EdgeNum = j;
                edgeSet.insert(edge);
            }
            else
            {
                // 중복 엣지
                edge                                      = *it;
                edge.pFaces[0]->pFaces[edge.face0EdgeNum] = pFace;            // 양쪽 면 연결
                pFace->pFaces[j]                          = edge.pFaces[0];   // 양쪽 면 연결
                edgeSet.erase(edge);
            }
        }
    }

    // 버텍스의 경계 및 regular 정보 설정
    for (Int32 i = 0; i < numVerts; ++i)
    {
        SDVertex* pVert = verts[i];
        SDFace*   pFace = pVert->pStartFace;
        do
        {
            pFace = pFace->GetNextFace(pVert);
        } while (pFace && pFace != pVert->pStartFace);

        pVert->bBoundary = (pFace == nullptr);
        pVert->bRegular  = (!pVert->bBoundary && pVert->CalcValence() == 6)
                       || (pVert->bBoundary && pVert->CalcValence() == 4);
    }

    // 세분화 수행
    for (Int32 i = 0; i < _level; ++i)
    {
        Vector<SDFace*> newFaces = {};
        newFaces.reserve(faces.size() * 4);   // 대략적인 크기 예약

        Vector<SDVertex*> newVerts = {};
        newVerts.reserve(verts.size() * 2);   // 대략적인 크기 예약

        // 새로운 버텍스 및 페이스 생성
        for (SDVertex* pVert: verts)
        {
            pVert->pChildVert            = arena.New<SDVertex>();
            pVert->pChildVert->bRegular  = pVert->bRegular;
            pVert->pChildVert->bBoundary = pVert->bBoundary;
            newVerts.emplace_back(pVert->pChildVert);
        }

        // 자식 페이스 생성
        for (SDFace* pFace: faces)
        {
            for (Int32 k = 0; k < 4; ++k)
            {
                pFace->pChildFaces[k] = arena.New<SDFace>();
                newFaces.emplace_back(pFace->pChildFaces[k]);
            }
        }

        // even 버텍스 위치 계산
        for (SDVertex* pVert: verts)
        {
            if (pVert->bBoundary)
            {
                pVert->pChildVert->point = WeightBoundary(pVert, 1.f / 8.f);
            }
            else
            {
                pVert->pChildVert->point = pVert->bRegular ? WeightOneRing(pVert, 1.f / 16.f)
                                                           : WeightOneRing(pVert, Beta(pVert->CalcValence()));
            }
        }

        // odd 에지 버텍스 위치 계산
        HashMap<SDEdge, SDVertex*, SDEdgeHash, SDEdgeEqual> edgeVertMap = {};
        for (SDFace* pFace: faces)
        {
            for (Int32 k = 0; k < 3; ++k)
            {
                SDEdge edge    = {};
                edge.pVerts[0] = Min(pFace->pVerts[k], pFace->pVerts[(k + 1) % 3]);
                edge.pVerts[1] = Max(pFace->pVerts[k], pFace->pVerts[(k + 1) % 3]);

                SDVertex* pVert = edgeVertMap[edge];
                if (!pVert)
                {
                    // odd 버텍스 생성
                    pVert = arena.New<SDVertex>();
                    newVerts.emplace_back(pVert);
                    pVert->bRegular   = true;
                    pVert->bBoundary  = (pFace->pFaces[k] == nullptr);
                    pVert->pStartFace = pFace->pChildFaces[3];

                    // odd 버텍스 위치 계산
                    const SDVertex* v0 = edge.pVerts[0];
                    const SDVertex* v1 = edge.pVerts[1];

                    if (pVert->bBoundary)
                    {
                        pVert->point = (v0->point + v1->point) * 0.5f;
                    }
                    else
                    {
                        const SDVertex* vOther    = pFace->GetOtherVertex(v0, v1);
                        const SDVertex* vOpposite = pFace->pFaces[k]->GetOtherVertex(v0, v1);
                        pVert->point              = (v0->point + v1->point) * (3.f / 8.f) + (vOther->point + vOpposite->point) * (1.f / 8.f);
                    }
                    edgeVertMap[edge] = pVert;
                }
            }
        }

        for (SDVertex* pVert: verts)
        {
            const Int32 idx               = pVert->pStartFace->CalcVertexIndex(pVert);
            pVert->pChildVert->pStartFace = pVert->pStartFace->pChildFaces[idx];
        }

        for (SDFace* pFace: faces)
        {
            for (Int32 j = 0; j < 3; ++j)
            {
                pFace->pChildFaces[3]->pFaces[j]           = pFace->pChildFaces[(j + 1) % 3];
                pFace->pChildFaces[j]->pFaces[(j + 1) % 3] = pFace->pChildFaces[3];

                SDFace* f2                                 = pFace->pFaces[j];
                pFace->pChildFaces[j]->pFaces[j]           = f2 ? f2->pChildFaces[f2->CalcVertexIndex(pFace->pVerts[j])] : nullptr;
                f2                                         = pFace->pFaces[(j + 2) % 3];
                pFace->pChildFaces[j]->pFaces[(j + 2) % 3] = f2 ? f2->pChildFaces[f2->CalcVertexIndex(pFace->pVerts[j])] : nullptr;
            }
        }

        for (SDFace* pFace: faces)
        {
            for (Int32 j = 0; j < 3; ++j)
            {
                pFace->pChildFaces[j]->pVerts[j] = pFace->pVerts[j]->pChildVert;

                SDEdge edge;
                edge.pVerts[0] = Min(pFace->pVerts[j], pFace->pVerts[(j + 1) % 3]);
                edge.pVerts[1] = Max(pFace->pVerts[j], pFace->pVerts[(j + 1) % 3]);

                SDVertex* pVert                            = edgeVertMap[edge];
                pFace->pChildFaces[j]->pVerts[(j + 1) % 3] = pVert;
                pFace->pChildFaces[(j + 1) % 3]->pVerts[j] = pVert;
                pFace->pChildFaces[3]->pVerts[j]           = pVert;
            }
        }

        verts.swap(newVerts);
        faces.swap(newFaces);
    }

    // 최종 위치 보정
    const Int32 finalNumVerts = static_cast<Int32>(verts.size());

    Vector<SDPoint> limits = {};
    limits.reserve(finalNumVerts);
    for (Int32 i = 0; i < finalNumVerts; ++i)
    {
        const SDVertex* pVert = verts[i];
        if (pVert->bBoundary)
        {
            limits.emplace_back(WeightBoundary(verts[i], 1.f / 5.f));
        }
        else
        {
            limits.emplace_back(WeightOneRing(verts[i], LoopGamma(verts[i]->CalcValence())));
        }
    }

    // 위치 적용
    for (Int32 i = 0; i < finalNumVerts; ++i)
    {
        verts[i]->point = limits[i];
    }

    // 포지션 + texCoord 배열 생성
    Vector<Vec3> finalPositions = {};
    finalPositions.reserve(finalNumVerts);

    Vector<Vec2> finalTexcoords = {};
    finalTexcoords.reserve(finalNumVerts);

    for (Int32 i = 0; i < finalNumVerts; ++i)
    {
        finalPositions.emplace_back(verts[i]->point.position);
        finalTexcoords.emplace_back(verts[i]->point.texcoord);
    }

    // 인덱스 배열 생성
    const Int32 finalNumFaces     = static_cast<Int32>(faces.size());
    const bool  bUseU32           = std::cmp_greater(finalNumVerts, Limits<UInt16>::kHighest);
    const Int32 finalNumIdxes     = finalNumFaces * 3;
    const Int32 finalIdxByteWidth = CalcIndicesByteWidth(finalNumIdxes, bUseU32);

    struct TangentFrame
    {
        Vector<Vec3> tangents;
        Vector<Vec3> bitangents;
    };
    // 버텍스 맵 생성
    HashMap<SDVertex*, Int32> usedVertsMap = {};
    for (Int32 i = 0; i < finalNumVerts; ++i)
    {
        usedVertsMap[verts[i]] = i;
    }

    // 인덱스 버퍼
    Vector<Byte> finalIdxData(finalIdxByteWidth);
    IndexWriter  indexWriter { finalIdxData, bUseU32 };
    for (Int32 i = 0; i < finalNumFaces; ++i)
    {
        for (Int32 j = 0; j < 3; ++j)
        {
            const Int32 idx = usedVertsMap[faces[i]->pVerts[j]];
            indexWriter.WriteAt(i * 3 + j, idx);
        }
    }

    LoopSubdivisionOutputWithTexcoords output = {};
    output.positions                          = std::move(finalPositions);
    output.texcoords                          = std::move(finalTexcoords);
    output.indices                            = std::move(finalIdxData);
    output.bUseU32                            = bUseU32;
    return output;
}

}   // namespace Jam