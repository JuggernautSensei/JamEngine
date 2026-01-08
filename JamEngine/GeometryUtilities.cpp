#include "pch.h"

#include "GeometryUtilities.h"

namespace jam
{

Int32 CalcNumIndices(const CMemView _idxData, const bool _bUseU32) noexcept
{
    const Int32 indexSize = _bUseU32 ? sizeof(UInt32) : sizeof(UInt16);
    return _idxData.GetByteWidth() / indexSize;
}

Int32 CalcNumVertices(const CMemView _vertData, const VertexLayout& _vl) noexcept
{
    const Int32 stride = _vl.GetStride();
    return _vertData.GetByteWidth() / stride;
}

Int32 CalcIndicesByteWidth(const Int32 _numIdxes, const bool _bUseU32) noexcept
{
    return _numIdxes * (_bUseU32 ? sizeof(UInt32) : sizeof(UInt16));
}

bool IsGreaterThanUInt16(const Int32 _value) noexcept
{
    return _value > 0xFFFF;
}

}   // namespace jam