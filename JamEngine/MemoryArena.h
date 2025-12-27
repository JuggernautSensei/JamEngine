#pragma once

namespace jam
{

class MemoryArena
{
public:
    explicit MemoryArena(const UInt32 _byteWidthPerArena)
        : m_byteWidthPerArena(_byteWidthPerArena)
    {
        JAM_ASSERT(_byteWidthPerArena > 0, "아레나의 크기는 0보다 커야 합니다.");
    }

    ~MemoryArena()
    {
        Reset();
    }

    MemoryArena(const MemoryArena&)            = delete;
    MemoryArena(MemoryArena&&)                 = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    MemoryArena& operator=(MemoryArena&&)      = delete;

    void* Alloc(const size_t _byteWidth)
    {
        constexpr size_t k_alignent = alignof(std::max_align_t);
        const size_t     byteWidth  = (_byteWidth + (k_alignent - 1)) & ~(k_alignent - 1);
        JAM_ASSERT(byteWidth <= m_byteWidthPerArena, "한 아레나에 할당할 수 있는 최대 크기를 초과했습니다.");

        if (m_pCur == nullptr)   // 아레나 초기화 필요
        {
            void* pMem = std::malloc(m_byteWidthPerArena + sizeof(Head));
            JAM_ASSERT(pMem, "메모리 할당에 실패했습니다.");
            m_pCurHead            = static_cast<Head*>(pMem);
            m_pCurHead->pPrevHead = nullptr;
            m_pCur                = static_cast<UInt8*>(pMem) + sizeof(Head);
            m_pEnd                = m_pCur + m_byteWidthPerArena;
        }

        if (m_pCur + byteWidth >= m_pEnd)
        {
            void* pMem = std::malloc(m_byteWidthPerArena + sizeof(Head));
            JAM_ASSERT(pMem, "메모리 할당에 실패했습니다.");
            Head* newHead      = static_cast<Head*>(pMem);
            newHead->pPrevHead = m_pCurHead;
            m_pCurHead         = newHead;
            m_pCur             = static_cast<UInt8*>(pMem) + sizeof(Head);
            m_pEnd             = m_pCur + m_byteWidthPerArena;
        }

        void* pRet = m_pCur;
        m_pCur     = m_pCur + byteWidth;
        return pRet;
    }

    template<typename T>
    [[nodiscard]] T* Alloc(const size_t _count = 1)
    {
        return static_cast<T*>(Alloc(_count * sizeof(T)));
    }

    template<typename T, typename... Args>
    [[nodiscard]] T* New(const size_t _count = 1, Args&&... _args)
    {
        T* pRet = Alloc<T>(_count);
        for (size_t i = 0; i < _count; ++i)
        {
            std::construct_at(&pRet[i], std::forward<Args>(_args)...);
        }
        return pRet;
    }

    void Reset()
    {
        while (m_pCurHead)
        {
            Head* pPrev = m_pCurHead->pPrevHead;
            std::free(m_pCurHead);
            m_pCurHead = pPrev;
        }
        m_pCur = nullptr;
        m_pEnd = nullptr;
    }

private:
    struct Head
    {
        Head* pPrevHead = nullptr;
    };

    Head*  m_pCurHead          = {};
    UInt8* m_pCur              = nullptr;
    UInt8* m_pEnd              = nullptr;
    UInt32 m_byteWidthPerArena = 0;
};

}   // namespace jam