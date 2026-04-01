#pragma once

namespace Jam
{

void* Malloc(size_t _size);
void* AlignedMalloc(size_t _size, size_t _alignment);
void* AlignedMallocAt(size_t _size, size_t _alignment, size_t _offset);
void* Realloc(void* _pMem, size_t _newSize);
void* AlignedRealloc(void* _pMem, size_t _newSize, size_t _alignment);
void* AlignedReallocAt(void* _pMem, size_t _newSize, size_t _alignment, size_t _offset);
void  Free(void* _pMem);
void  AlignedFree(void* _pMem, size_t _alignment);

void* New(size_t _size);
void* New(size_t _count, size_t _size);
void* AlignedNew(size_t _size, size_t _alignment);
void* AlignedNewAt(size_t _size, size_t _alignment, size_t _offset);

}   // namespace Jam