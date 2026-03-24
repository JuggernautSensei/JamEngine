#include "pch.h"

#include "GeometryUtilities.h"

namespace Jam
{

Int32 CalcNumIndices(
    const MemoryView _idxData,
    const bool         _bUseU32) noexcept
{
    const SizeT indexSize = NumericCast<SizeT>(_bUseU32 ? sizeof(UInt32) : sizeof(UInt16));
    return NumericCast<Int32>(_idxData.GetByteWidth() / indexSize);
}

Int32 CalcNumVertices(
    const MemoryView  _vertData,
    const VertexLayout& _vl) noexcept
{
    const Int32 stride = _vl.GetStride();
    return NumericCast<Int32>(_vertData.GetByteWidth() / stride);
}

Int32 CalcIndicesByteWidth(
    const Int32 _numIdxes,
    const bool  _bUseU32) noexcept
{
    const SizeT indexSize = NumericCast<SizeT>(_bUseU32 ? sizeof(UInt32) : sizeof(UInt16));
    return NumericCast<Int32>(_numIdxes * indexSize);
}

bool NeedU32Index(
    const MemoryView  _vertData,
    const VertexLayout& _vl) noexcept
{
    const Int32 numVerts = CalcNumVertices(_vertData, _vl);
    return NeedU32Index(numVerts);
}

bool NeedU32Index(
    const Int32 _numVerts) noexcept
{
    return CmpGreater(_numVerts, Limits<UInt16>::k_max);
}

}   // namespace Jam