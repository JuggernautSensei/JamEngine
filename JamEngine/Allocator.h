#pragma once

namespace Jam
{

struct Allocator
{
    [[nodiscard]] void* Alloc(
        const USizeT _size) const
    {
        return ::operator new(_size);
    }

    void Free(
        void* const _ptr) const
    {
        ::operator delete(_ptr);
    }

    template<typename T, typename... TArgs>
    [[nodiscard]] T* New(
        TArgs&&... _args) const
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T(std::forward<TArgs>(_args)...);
    }

    template<typename T>
    void Delete(T* const _ptr) const
    {
        if (_ptr)
        {
            _ptr->~T();
            Free(_ptr);
        }
    }
};

struct MemoryAllocator
{
    [[nodiscard]] void* Alloc(
        const USizeT _size) const
    {
        return Malloc(_size);
    }

    [[nodiscard]] void* ReAlloc(
        void* const  _ptr,
        const USizeT _newSize) const
    {
        return Realloc(_ptr, _newSize);
    }

    void Free(void* const _ptr) const
    {
        Free(_ptr);
    }
};

}   // namespace Jam
