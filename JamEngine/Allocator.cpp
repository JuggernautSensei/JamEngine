#include "pch.h"
#include "Allocator.h"

namespace Jam
{

void* Malloc(
    const size_t _size)
{
    return ::malloc(_size);
}

void* AlignedMalloc(
    const size_t _size,
    const size_t _alignment)
{
    return ::_aligned_malloc(_size, _alignment);
}

void* AlignedMallocAt(
    const size_t _size,
    const size_t _alignment,
    const size_t _offset)
{
    return ::_aligned_offset_malloc(_size, _alignment, _offset);
}

void* Realloc(
    void*        _pMem,
    const size_t _newSize)
{
    return ::realloc(_pMem, _newSize);
}

void* AlignedRealloc(
    void*        _pMem,
    const size_t _newSize,
    const size_t _alignment)
{
    return ::_aligned_realloc(_pMem, _newSize, _alignment);
}

void* AlignedReallocAt(
    void*        _pMem,
    const size_t _newSize,
    const size_t _alignment,
    const size_t _offset)
{
    return ::_aligned_offset_realloc(_pMem, _newSize, _alignment, _offset);
}

void Free(void* _pMem)
{
    ::free(_pMem);
}

void AlignedFree(
    void*        _pMem,
    const size_t _alignment)
{
    ::_aligned_free(_pMem);
}

void* New(
    const size_t _size)
{
    return ::operator new(_size);
}

void* New(
    const size_t _count,
    const size_t _size)
{
    return ::operator new(_count * _size);
}

void* AlignedNew(
    const size_t _size,
    const size_t _alignment)
{
    return ::operator new(_size, static_cast<std::align_val_t>(_alignment));
}

void* AlignedNewAt(
    const size_t _size,
    const size_t _alignment,
    const size_t)
{
    // offset is not supported in C++ new operator
    return ::operator new(_size, static_cast<std::align_val_t>(_alignment));
}

}   // namespace Jam