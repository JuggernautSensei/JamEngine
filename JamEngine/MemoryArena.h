#pragma once

namespace Jam
{

class MemoryArena
{
public:
    explicit MemoryArena(SizeT _byteWidthPerArena);

    ~MemoryArena();

    MemoryArena(const MemoryArena&)            = delete;
    MemoryArena(MemoryArena&&)                 = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    MemoryArena& operator=(MemoryArena&&)      = delete;

    void* Alloc(SizeT _byteWidth);

    template<typename T>
    T* Alloc()
    {
        return static_cast<T*>(Alloc(sizeof(T)));
    }

    template<typename T>
    T* Alloc(const SizeT _count)
    {
        return static_cast<T*>(Alloc(_count * sizeof(T)));
    }

    template<typename T, typename... TArgs>
    T* New(TArgs&&... _args)
    {
        T* pRet = Alloc<T>();
        std::construct_at(pRet, std::forward<TArgs>(_args)...);
        return pRet;
    }

    template<typename T, typename... TArgs>
    T* New(const SizeT _count, TArgs&&... _args)
    {
        T* pRet = Alloc<T>(_count);
        for (SizeT i = 0; i < _count; ++i)
        {
            std::construct_at(&pRet[i], std::forward<TArgs>(_args)...);
        }
        return pRet;
    }

    void Reset();

private:
    struct Head
    {
        Head* pPrevHead = nullptr;
    };

    Head* m_pCurHead          = {};
    Byte* m_pCur              = nullptr;
    Byte* m_pEnd              = nullptr;
    SizeT m_byteWidthPerArena = 0;
};

}   // namespace Jam