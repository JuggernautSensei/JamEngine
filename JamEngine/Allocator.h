#pragma once

namespace Jam
{

[[nodiscard]] void* Malloc(size_t _size);
[[nodiscard]] void* AlignedMalloc(size_t _size, size_t _alignment);
[[nodiscard]] void* AlignedMallocAt(size_t _size, size_t _alignment, size_t _offset);
[[nodiscard]] void* Realloc(void* _pMem, size_t _newSize);
[[nodiscard]] void* AlignedRealloc(void* _pMem, size_t _newSize, size_t _alignment);
[[nodiscard]] void* AlignedReallocAt(void* _pMem, size_t _newSize, size_t _alignment, size_t _offset);
void                Free(void* _pMem);
void                AlignedFree(void* _pMem, size_t _alignment);

[[nodiscard]] void* New(size_t _size);
[[nodiscard]] void* New(size_t _count, size_t _size);
[[nodiscard]] void* AlignedNew(size_t _size, size_t _alignment);
[[nodiscard]] void* AlignedNewAt(size_t _size, size_t _alignment, size_t _offset);

}   // namespace Jam

#define JAM_STACK_ALLOC(_size) _alloca(_size)
