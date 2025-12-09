#include "pch.h"

#include "LoopSubdivision.h"

#include "BaseUtilities.h"
#include "MemView.h"
#include "MemoryArena.h"

// 루프 세분화 알고리즘
// https://pbr-book.org/3ed-2018/Shapes/Subdivision_Surfaces
// https://en.wikipedia.org/wiki/Loop_subdivision_surface

namespace jam
{

struct SDFace;
struct SDVertex;

/// @brief 정점과 텍스처 좌표를 나타내는 구조체
struct SDPoint
{
    Vec3 position = Vec3::Zero;
    Vec2 texcoord = Vec2::Zero;
};

struct SDVertex
{
    [[nodiscard]] Int32                CalcValence() const;
    [[nodiscard]] std::vector<SDPoint> OneRing() const;

    SDPoint   point;                  // 정점의 위치 및 텍스처 좌표
    SDFace*   pStartFace = nullptr;   // 임의의 면
    SDVertex* pChildVert = nullptr;   // 세분화 후 생성된 자식 버텍스
    bool      bRegular   = false;     // 정점이 Regular한지 여부
    bool      bBoundary  = false;     // 경계 버텍스 여부
};

struct SDFace
{
    [[nodiscard]] Int32 CalcVertexIndex(const SDVertex* vert) const
    {
        for (Int32 i = 0; i < 3; ++i)
        {
            if (pVerts[i] == vert)
            {
                return i;
            }
        }
        JAM_ASSERT(0, "Basic logic error in SDFace::CalcVertexIndex()");
        return -1;
    }

    [[nodiscard]] SDFace* GetNextFace(const SDVertex* vert) const
    {
        return pFaces[CalcVertexIndex(vert)];
    }

    [[nodiscard]] SDFace* GetPrevFace(const SDVertex* vert) const
    {
        return pFaces[(CalcVertexIndex(vert) + 2) % 3];
    }

    [[nodiscard]] SDVertex* GetNextVertex(const SDVertex* vert) const
    {
        return pVerts[(CalcVertexIndex(vert) + 1) % 3];
    }

    [[nodiscard]] SDVertex* GetPrevVertex(const SDVertex* vert) const
    {
        return pVerts[(CalcVertexIndex(vert) + 2) % 3];
    }

    [[nodiscard]] SDVertex* GetOtherVertex(const SDVertex* v0, const SDVertex* v1) const
    {
        for (Int32 i = 0; i < 3; ++i)
        {
            if (pVerts[i] != v0 && pVerts[i] != v1)
            {
                return pVerts[i];
            }
        }
        JAM_ASSERT(0, "Basic logic error in SDFace::GetOtherVertex()");
        return nullptr;
    }

    SDVertex* pVerts[3]      = { nullptr, nullptr, nullptr };
    SDFace*   pFaces[3]      = { nullptr, nullptr, nullptr };
    SDFace*   pChildFaces[4] = { nullptr, nullptr, nullptr, nullptr };
};

struct SDEdge
{
    bool operator<(const SDEdge& other) const
    {
        if (pVerts[0] == other.pVerts[0])
            return pVerts[1] < other.pVerts[1];
        return pVerts[0] < other.pVerts[0];
    }

    SDVertex* pVerts[2]    = { nullptr, nullptr };
    SDFace*   pFaces[2]    = { nullptr, nullptr };
    Int32     face0EdgeNum = -1;
};

Int32 SDVertex::CalcValence() const
{
    SDFace* f = pStartFace;
    if (!bBoundary)
    {
        Int32 nf = 1;
        while ((f = f->GetNextFace(this)) != pStartFace)
            ++nf;
        return nf;
    }
    else
    {
        Int32 nf = 1;
        while ((f = f->GetNextFace(this)))
            ++nf;
        f = pStartFace;
        while ((f = f->GetPrevFace(this)))
            ++nf;
        return nf + 1;
    }
}

std::vector<SDPoint> SDVertex::OneRing() const
{
    Int32 valence = CalcValence();

    std::vector<SDPoint> pRing;
    pRing.reserve(valence);

    if (!bBoundary)
    {
        SDFace* face = pStartFace;
        do
        {
            pRing.push_back(face->GetNextVertex(this)->point);
            face = face->GetNextFace(this);
        } while (face != pStartFace);
    }
    else
    {
        SDFace *face = pStartFace, *f2;
        while ((f2 = face->GetNextFace(this)))
            face = f2;
        pRing.push_back(face->GetNextVertex(this)->point);
        do
        {
            pRing.push_back(face->GetPrevVertex(this)->point);
            face = face->GetPrevFace(this);
        } while (face);
    }
    return pRing;
}

static float Beta(const Int32 _valence)
{
    return _valence == 3 ? 3.f / 16.f : 3.f / (8.f * static_cast<float>(_valence));
}

static float LoopGamma(const Int32 _valence)
{
    return 1.f / (static_cast<float>(_valence) + 3.f / (8.f * Beta(_valence)));
}

static SDPoint WeightOneRing(const SDVertex* vert, const float Beta)
{
    Int32                CalcValence = vert->CalcValence();
    std::vector<SDPoint> pRing       = vert->OneRing();

    SDPoint point;
    point.position = vert->point.position * (1 - static_cast<float>(CalcValence) * Beta);
    point.texcoord = vert->point.texcoord * (1 - static_cast<float>(CalcValence) * Beta);

    for (Int32 i = 0; i < CalcValence; ++i)
    {
        point.position += pRing[i].position * Beta;
        point.texcoord += pRing[i].texcoord * Beta;
    }
    return point;
}

static SDPoint WeightBoundary(const SDVertex* vert, const float Beta)
{
    Int32                CalcValence = vert->CalcValence();
    std::vector<SDPoint> pRing       = vert->OneRing();

    SDPoint point;
    point.position = vert->point.position * (1 - 2 * Beta);
    point.texcoord = vert->point.texcoord * (1 - 2 * Beta);
    point.position += pRing[0].position * Beta;
    point.texcoord += pRing[0].texcoord * Beta;
    point.position += pRing[CalcValence - 1].position * Beta;
    point.texcoord += pRing[CalcValence - 1].texcoord * Beta;
    return point;
}

///////////////////////////////////////////////////////////

LoopSubdivisionOutput LoopSubdivide(
    std::span<const Vec3> _positions,
    std::span<const Vec2> _texcoordsOrEmpty,
    ConstMemView          _indexData,
    bool                  _bUseU32,
    const Int32           _level)
{
    JAM_ASSERT(!_positions.empty(), "세분화할 메쉬의 버텍스가 비어있습니다.");
    JAM_ASSERT(_indexData.IsValid(), "세분화할 메쉬의 인덱스 데이터가 유효하지 않습니다.");

    Int32 numVerts    = NarrowCast<Int32>(_positions.size());
    Int32 indexStride = _bUseU32 ? 4 : 2;
    Int32 numIndices  = NarrowCast<Int32>(_indexData.GetByteWidth() / indexStride);
    Int32 numFaces    = numIndices / 3;
    bool  bHasUV      = !_texcoordsOrEmpty.empty();

    // 버텍스 및 페이스
    std::vector<SDVertex*> verts;
    verts.reserve(numVerts);   // 대략적인 용량 예약

    std::vector<SDFace*> faces;
    faces.reserve(numFaces);   // 대략적인 용량 예약

    // 초기 버텍스 생성
    std::unique_ptr<SDVertex[]> initialVerts = std::make_unique<SDVertex[]>(numVerts);
    for (Int32 i = 0; i < numVerts; ++i)
    {
        initialVerts[i].point.position = _positions[i];
        initialVerts[i].point.texcoord = bHasUV ? _texcoordsOrEmpty[i] : Vec2::Zero;
        verts.push_back(&initialVerts[i]);
    }

    // 초기 페이스 생성 (메모리만 할당)
    std::unique_ptr<SDFace[]> initialFaces = std::make_unique<SDFace[]>(numFaces);
    for (Int32 i = 0; i < numFaces; ++i)
    {
        faces.push_back(&initialFaces[i]);
    }

    // 페이스의 버텍스 정보 설정
    const Byte* pIndexData = _indexData.GetDataPtr();
    for (Int32 i = 0; i < numFaces; ++i)
    {
        SDFace* pFace = faces[i];
        for (Int32 j = 0; j < 3; ++j)
        {
            const Byte*  pIndex = pIndexData + (i * 3 + j) * indexStride;
            const UInt32 index  = _bUseU32 ? *reinterpret_cast<const UInt16*>(pIndex) : *reinterpret_cast<const UInt32*>(pIndex);
            SDVertex*    pVert  = verts[index];
            pFace->pVerts[j]    = pVert;
            pVert->pStartFace   = pFace;   // 임의의 면 설정
        }
    }

    // 엣지 정보 설정
    std::set<SDEdge> edgeSet;
    for (Int32 i = 0; i < numFaces; ++i)
    {
        SDFace* pFace = faces[i];
        for (Int32 edgeNum = 0; edgeNum < 3; ++edgeNum)
        {
            const Int32 v0 = edgeNum;
            const Int32 v1 = (edgeNum + 1) % 3;

            SDEdge edge;
            edge.pVerts[0] = Min(pFace->pVerts[v0], pFace->pVerts[v1]);
            edge.pVerts[1] = Max(pFace->pVerts[v0], pFace->pVerts[v1]);

            auto edgeIt = edgeSet.find(edge);
            if (edgeIt == edgeSet.end())
            {
                // 새로운 엣지
                edge.pFaces[0]    = pFace;
                edge.face0EdgeNum = edgeNum;
                edgeSet.insert(edge);
            }
            else
            {
                // 중복 엣지
                edge                                      = *edgeIt;
                edge.pFaces[0]->pFaces[edge.face0EdgeNum] = pFace;            // 양쪽 면 연결
                pFace->pFaces[edgeNum]                    = edge.pFaces[0];   // 양쪽 면 연결
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
        pVert->bRegular  = (!pVert->bBoundary && pVert->CalcValence() == 6) || (pVert->bBoundary && pVert->CalcValence() == 4);
    }

    // 세분화 수행
    MemoryArena arena { 1024 };
    for (Int32 i = 0; i < _level; ++i)
    {
        std::vector<SDFace*> newFaces;
        newFaces.reserve(faces.size() * 4);

        std::vector<SDVertex*> newVerts;
        newVerts.reserve(verts.size() * 2);

        // 새로운 버텍스 및 페이스 생성
        for (SDVertex* vert: verts)
        {
            vert->pChildVert            = arena.New<SDVertex>();
            vert->pChildVert->bRegular  = vert->bRegular;
            vert->pChildVert->bBoundary = vert->bBoundary;
            newVerts.push_back(vert->pChildVert);
        }

        for (SDFace* face: faces)
        {
            for (Int32 k = 0; k < 4; ++k)
            {
                face->pChildFaces[k] = arena.Alloc<SDFace>();
                newFaces.push_back(face->pChildFaces[k]);
            }
        }

        // even 버텍스 위치 계산
        for (SDVertex* pVert: verts)
        {
            if (!pVert->bBoundary)
            {
                pVert->pChildVert->point = pVert->bRegular ? WeightOneRing(pVert, 1.f / 16.f)
                                                           : WeightOneRing(pVert, Beta(pVert->CalcValence()));
            }
            else
            {
                pVert->pChildVert->point = WeightBoundary(pVert, 1.f / 8.f);
            }
        }

        // odd 에지 버텍스 위치 계산
        std::map<SDEdge, SDVertex*> edgeVertMap;
        for (SDFace* pFace: faces)
        {
            for (Int32 k = 0; k < 3; ++k)
            {
                SDEdge edge;
                edge.pVerts[0] = Min(pFace->pVerts[k], pFace->pVerts[(k + 1) % 3]);
                edge.pVerts[1] = Max(pFace->pVerts[k], pFace->pVerts[(k + 1) % 3]);

                SDVertex* vert = edgeVertMap[edge];
                if (!vert)
                {
                    // odd 버텍스 생성
                    vert = arena.Alloc<SDVertex>();
                    newVerts.push_back(vert);
                    vert->bRegular   = true;
                    vert->bBoundary  = (pFace->pFaces[k] == nullptr);
                    vert->pStartFace = pFace->pChildFaces[3];

                    // odd 버텍스 위치 계산
                    if (vert->bBoundary)
                    {
                        vert->point.position = 0.5f * (edge.pVerts[0]->point.position + edge.pVerts[1]->point.position);
                        vert->point.texcoord = 0.5f * (edge.pVerts[0]->point.texcoord + edge.pVerts[1]->point.texcoord);
                    }
                    else
                    {
                        vert->point.position = (3.f / 8.f) * (edge.pVerts[0]->point.position + edge.pVerts[1]->point.position) + (1.f / 8.f) * (pFace->GetOtherVertex(edge.pVerts[0], edge.pVerts[1])->point.position + pFace->pFaces[k]->GetOtherVertex(edge.pVerts[0], edge.pVerts[1])->point.position);
                        vert->point.texcoord = (3.f / 8.f) * (edge.pVerts[0]->point.texcoord + edge.pVerts[1]->point.texcoord) + (1.f / 8.f) * (pFace->GetOtherVertex(edge.pVerts[0], edge.pVerts[1])->point.texcoord + pFace->pFaces[k]->GetOtherVertex(edge.pVerts[0], edge.pVerts[1])->point.texcoord);
                    }
                    edgeVertMap[edge] = vert;
                }
            }
        }

        for (SDVertex* pVert: verts)
        {
            Int32 vertNum                 = pVert->pStartFace->CalcVertexIndex(pVert);
            pVert->pChildVert->pStartFace = pVert->pStartFace->pChildFaces[vertNum];
        }

        for (SDFace* pFace: faces)
        {
            for (Int32 j = 0; j < 3; ++j)
            {
                pFace->pChildFaces[3]->pFaces[j]           = pFace->pChildFaces[(j + 1) % 3];
                pFace->pChildFaces[j]->pFaces[(j + 1) % 3] = pFace->pChildFaces[3];

                SDFace* f2                       = pFace->pFaces[j];
                pFace->pChildFaces[j]->pFaces[j] = f2 ? f2->pChildFaces[f2->CalcVertexIndex(pFace->pVerts[j])] : nullptr;

                f2                                         = pFace->pFaces[(j + 2) % 3];
                pFace->pChildFaces[j]->pFaces[(j + 2) % 3] = f2 ? f2->pChildFaces[f2->CalcVertexIndex(pFace->pVerts[j])] : nullptr;
            }
        }

        for (SDFace* face: faces)
        {
            for (Int32 j = 0; j < 3; ++j)
            {
                face->pChildFaces[j]->pVerts[j] = face->pVerts[j]->pChildVert;

                SDEdge edge;
                edge.pVerts[0] = Min(face->pVerts[j], face->pVerts[(j + 1) % 3]);
                edge.pVerts[1] = Max(face->pVerts[j], face->pVerts[(j + 1) % 3]);

                SDVertex* pVert                           = edgeVertMap[edge];
                face->pChildFaces[j]->pVerts[(j + 1) % 3] = pVert;
                face->pChildFaces[(j + 1) % 3]->pVerts[j] = pVert;
                face->pChildFaces[3]->pVerts[j]           = pVert;
            }
        }

        verts.swap(newVerts);
        faces.swap(newFaces);
    }

    // 최종 위치 보정
    numVerts                          = NarrowCast<Int32>(verts.size());
    std::unique_ptr<SDPoint[]> pLimit = std::make_unique<SDPoint[]>(numVerts);
    for (Int32 i = 0; i < numVerts; ++i)
    {
        if (verts[i]->bBoundary)
            pLimit[i] = WeightBoundary(verts[i], 1.f / 5.f);
        else
            pLimit[i] = WeightOneRing(verts[i], LoopGamma(verts[i]->CalcValence()));
    }

    // 위치 적용
    for (Int32 i = 0; i < numVerts; ++i)
    {
        verts[i]->point = pLimit[i];
    }

    // 포지션 + UVs 배열 생성
    LoopSubdivisionOutput output;
    output.positions.resize(numVerts);
    output.texcoords.resize(numVerts);
    for (Int32 i = 0; i < numVerts; ++i)
    {
        output.positions[i] = verts[i]->point.position;
        output.texcoords[i] = verts[i]->point.texcoord;
    }

    // 인덱스 배열 생성
    numFaces       = NarrowCast<Int32>(faces.size());
    output.bUseU32 = numVerts > 0xFFFF;
    indexStride    = output.bUseU32 ? 4 : 2;
    output.indices.resize(numFaces * 3 * indexStride);

    std::map<SDVertex*, Int32> usedVertsMap;
    for (Int32 i = 0; i < numVerts; ++i)
    {
        usedVertsMap[verts[i]] = i;
    }

    Byte* pIndices = output.indices.data();
    for (Int32 i = 0; i < numFaces; ++i)
    {
        for (Int32 j = 0; j < 3; ++j)
        {
            const Int32 index = usedVertsMap[faces[i]->pVerts[j]];
            Byte*       pDst  = pIndices + (i * 3 + j) * indexStride;
            if (output.bUseU32)
            {
                *reinterpret_cast<UInt16*>(pDst) = NarrowCast<UInt16>(index);
            }
            else
            {
                *reinterpret_cast<UInt32*>(pDst) = static_cast<UInt32>(index);
            }
        }
    }

    return output;
}

}   // namespace jam