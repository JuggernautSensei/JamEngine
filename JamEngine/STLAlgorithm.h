#pragma once

// STL friendly algorithms and utilities

namespace Jam
{

template<typename T>
void VectorSplice(
    Vector<T>&  _vec,
    const SizeT _dstIdx,
    const SizeT _srcIdx,
    const SizeT _count = 1)
{
    if (_dstIdx == _srcIdx || _count == 0)
    {
        return;
    }

    auto first = _vec.begin();
    if (_dstIdx < _srcIdx)
    {
        std::rotate(first + _dstIdx, first + _srcIdx, first + _srcIdx + _count);
    }
    else   // _dstIdx > _srcIdx
    {
        std::rotate(first + _srcIdx, first + _srcIdx + _count, first + _dstIdx);
    }
}

template<typename T>
void VectorErase(
    Vector<T>&  _vec,
    const SizeT _idx,
    const SizeT _count = 1)
{
    JAM_ASSERT(_idx < _vec.size());
    JAM_ASSERT(_idx + _count <= _vec.size());

    if (_count == 0)
    {
        return;
    }

    auto begin = _vec.begin() + _idx;
    auto end   = begin + _count;
    _vec.erase(begin, end);
}

template<typename T>
void VectorInsert(
    Vector<T>&  _vec,
    const SizeT _idx,
    const T&    _value)
{
    JAM_ASSERT(_idx <= _vec.size());
    _vec.insert(_vec.begin() + _idx, _value);
}

}   // namespace Jam