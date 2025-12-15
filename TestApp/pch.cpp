#include "pch.h"
#include <EASTL/allocator.h>
#include <new>

// EASTL 커스텀 allocator 구현
void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new[](size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    return aligned_alloc(alignment, size);
#endif
}
