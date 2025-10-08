#pragma once

namespace jam
{

/// @brief 메모리 아레나
/// 대량의 메모리를 미리 할당(아레나)하고, 그 안에서 작은 단위의 할당 요청을 처리합니다.
/// 개별 해제가 불가능하며, Reset()을 통해 전체 아레나의 메모리를 한 번에 해제하는 방식입니다.
/// 수명이 짧은 객체들을 대량으로 생성하고 소멸시킬 때 new/delete 오버헤드를 줄여 성능을 향상시킵니다.
class MemoryArena
{
public:
    /// @brief 생성자
    /// @param _byteWidthPerArena 할당할 각 아레나의 크기(바이트)
    explicit MemoryArena(const UInt32 _byteWidthPerArena)
        : m_byteWidthPerArena(_byteWidthPerArena)
    {
        JAM_ASSERT(_byteWidthPerArena > 0, "아레나의 크기는 0보다 커야 합니다.");
    }

    /// @brief 소멸자
    /// 모든 할당된 아레나 메모리를 해제합니다.
    ~MemoryArena()
    {
        Reset();
    }

    /// @brief 복사 및 이동 금지
    MemoryArena(const MemoryArena&)            = delete;
    MemoryArena(MemoryArena&&)                 = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    MemoryArena& operator=(MemoryArena&&)      = delete;

    /// @brief 메모리 블록 할당
    /// @param _byteWidth 할당할 메모리의 크기(바이트)
    /// @return 할당된 메모리 블록의 시작 주소
    void* Alloc(const size_t _byteWidth)
    {
        constexpr size_t k_alignent = alignof(std::max_align_t);
        size_t           byteWidth  = (_byteWidth + (k_alignent - 1)) & ~(k_alignent - 1);
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

    /// @brief 특정 타입 객체의 메모리 할당
    /// @tparam T 할당할 객체의 타입
    /// @param _count 할당할 객체의 개수 (기본값: 1)
    /// @return 할당된 객체의 시작 주소
    /// @note 객체의 생성자나 소멸자는 호출하지 않습니다.
    template<typename T>
    JAM_NODISCARD T* Alloc(const size_t _count = 1)
    {
        return static_cast<T*>(Alloc(_count * sizeof(T)));
    }

    /// @brief 객체 생성 및 할당
    /// @tparam T 생성할 객체의 타입
    /// @tparam Args 생성자 인자의 타입들
    /// @param _count 생성할 객체의 개수
    /// @param _args 생성자에 전달할 인자들
    template<typename T, typename... Args>
    JAM_NODISCARD T* New(const size_t _count = 1, Args&&... _args = {})
    {
        T* pRet = Alloc<T>(_count);
        for (size_t i = 0; i < _count; ++i)
        {
            std::construct_at(&pRet[i], std::forward<Args>(_args)...);
        }
        return pRet;
    }

    /// @brief 모든 할당된 아레나 메모리 해제
    /// 모든 아레나를 해제하고 내부 포인터를 초기 상태로 되돌립니다.
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
    /// @brief 여러 아레나를 연결하기 위한 헤더 구조체
    struct Head
    {
        Head* pPrevHead = nullptr;
    };

    Head*  m_pCurHead          = {};
    UInt8* m_pCur              = nullptr;
    UInt8* m_pEnd              = nullptr;
    UInt32 m_byteWidthPerArena = 0;
};

/// @brief MemoryArena를 사용하는 STL 컨테이너용 할당자
/// @tparam T 할당할 객체의 타입
template<typename T>
struct ArenaAllocator
{
    using value_type = T;

    /// @brief 생성자
    /// @param arena 메모리를 할당할 MemoryArena 객체
    explicit ArenaAllocator(MemoryArena& arena)
        : m_arena(&arena)
    {
    }

    /// @brief 다른 타입의 ArenaAllocator로부터 생성 (재바인딩용)
    template<typename U>
    explicit ArenaAllocator(const ArenaAllocator<U>& other) noexcept
        : m_arena(other.m_arena)
    {
    }

    /// @brief 메모리 할당
    /// MemoryArena로부터 n개의 T 객체를 위한 메모리를 할당합니다.
    T* allocate(const size_t n)
    {
        return static_cast<T*>(m_arena->Alloc(n * sizeof(T)));
    }

    /// @brief 메모리 해제 (아무 작업도 하지 않음)
    /// ArenaAllocator의 특성상 개별 해제는 지원하지 않습니다.
    /// 메모리는 MemoryArena::Reset()을 통해 전체가 해제됩니다.
    void deallocate(T*, size_t) noexcept
    {
    }

    /// @brief 두 할당자가 동일한 MemoryArena를 사용하는지 비교
    template<typename U>
    bool operator==(const ArenaAllocator<U>& other) const noexcept
    {
        return m_arena == other.m_arena;
    }

    /// @brief 두 할당자가 다른 MemoryArena를 사용하는지 비교
    template<typename U>
    bool operator!=(const ArenaAllocator<U>& other) const noexcept
    {
        return !(*this == other);
    }

    MemoryArena* m_arena;
};

}   // namespace jam