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
    const SizeT _index,
    const SizeT _count = 1)
{
    JAM_ASSERT(_index < _vec.size(), "Index out of bounds");
    JAM_ASSERT(_index + _count <= _vec.size(), "Count exceeds vector size");

    if (_count == 0)
    {
        return;
    }

    auto begin = _vec.begin() + _index;
    auto end   = begin + _count;
    _vec.erase(begin, end);
}

template<typename T>
void VectorInsert(
    Vector<T>&  _vec,
    const SizeT _index,
    const T&    _value)
{
    JAM_ASSERT(_index <= _vec.size(), "Index out of bounds");
    _vec.insert(_vec.begin() + _index, _value);
}

template<typename T>
    requires(std::equality_comparable<T>)
bool VectorContains(
    Vector<T>& _vec,
    const T&   _value)
{
    for (const T& v: _vec)
    {
        if (v == _value)
        {
            return true;
        }
    }

    return false;
}

template<typename T, typename Predicate>
    requires(std::is_invocable_r_v<bool, Predicate, const T&> || std::is_invocable_r_v<bool, Predicate, T&> || std::is_invocable_r_v<bool, Predicate, T>)
bool VectorContainsIf(
    Vector<T>&  _vec,
    Predicate&& _predicate)
{
    for (const T& v: _vec)
    {
        if (_predicate(v))
        {
            return true;
        }
    }

    return false;
}

template<typename T, SizeT N, typename Predicate>
    requires(std::is_invocable_r_v<bool, Predicate, const T&> || std::is_invocable_r_v<bool, Predicate, T&> || std::is_invocable_r_v<bool, Predicate, T>)
SizeT ArrayEraseIf(
    Array<T, N>& _array,
    Predicate&&  _predicate)
{
    Array<T, N> newArray;
    Int32       newSize   = 0;
    Int32       numErased = 0;
    for (Int32 i = 0; i < N; ++i)
    {
        if (_predicate(_array[i]))
        {
            ++numErased;
        }
        else
        {
            newArray[newSize++] = _array[i];
        }
    }
    std::swap(_array, newArray);
    return numErased;
}

}   // namespace Jam