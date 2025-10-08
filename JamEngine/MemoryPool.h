#pragma once

namespace jam
{

/// @brief 고정 길이 메모리 풀
class MemoryPool
{
public:
    /// @brief 생성자
    /// @param _bytePerBlock 각 블록의 크기 (최소 Node 크기)
    /// @param _blocksPerPool 각 풀에 포함될 블록 수
    explicit MemoryPool(const uint32_t _bytePerBlock, const uint32_t _blocksPerPool)
        : m_bytePerBlock(_bytePerBlock >= sizeof(Node) ? _bytePerBlock : sizeof(Node))
        , m_blocksPerPool(_blocksPerPool)
    {
        m_bytePerBlock = (m_bytePerBlock + (alignof(std::max_align_t) - 1)) & ~(alignof(std::max_align_t) - 1);
    }

    /// @brief 소멸자
    /// 모든 할당된 메모리를 해제합니다.
    ~MemoryPool()
    {
        Reset();
    }

    /// @brief 복사 및 이동 금지
    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool(MemoryPool&&)                 = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool& operator=(MemoryPool&&)      = delete;

    /// @brief 메모리 블록 할당
    void* Alloc()
    {
        if (m_pCurBlock == nullptr)   // 풀 초기화 필요
        {
            uint8_t* pMem      = static_cast<uint8_t*>(std::malloc(sizeof(Node) + m_bytePerBlock * m_blocksPerPool));
            m_pCurBlock        = reinterpret_cast<Node*>(pMem + sizeof(Node));
            m_pEndBlock        = reinterpret_cast<Node*>(pMem + sizeof(Node) + m_blocksPerPool * m_bytePerBlock);
            m_pPoolHead        = reinterpret_cast<Node*>(pMem);
            m_pPoolHead->pNext = nullptr;
        }

        if (m_pFreeBlock)   // 프리 블록이 있을 경우 재사용
        {
            Node* pRet   = m_pFreeBlock;
            m_pFreeBlock = m_pFreeBlock->pNext;
            pRet->pNext  = nullptr;
            return pRet;
        }
        else   // 프리 블록이 없으면 새 블록 할당
        {
            if (m_pCurBlock >= m_pEndBlock)   // 새로운 풀 할당 필요
            {
                uint8_t* pMem = static_cast<uint8_t*>(std::malloc(sizeof(Node) + m_bytePerBlock * m_blocksPerPool));
                m_pCurBlock   = reinterpret_cast<Node*>(pMem + sizeof(Node));
                m_pEndBlock   = reinterpret_cast<Node*>(pMem + sizeof(Node) + m_blocksPerPool * m_bytePerBlock);

                // 새 풀을 링크드 리스트에 추가
                Node* pNewPoolHead  = reinterpret_cast<Node*>(pMem);
                pNewPoolHead->pNext = m_pPoolHead;
                m_pPoolHead         = pNewPoolHead;
            }

            // 현재 블록에서 새 블록 할당
            Node* pRet  = m_pCurBlock;
            m_pCurBlock = reinterpret_cast<Node*>(reinterpret_cast<uint8_t*>(m_pCurBlock) + m_bytePerBlock);
            pRet->pNext = nullptr;
            return pRet;
        }
    }

    /// @brief 메모리 블록 해제
    void Free(void* p)
    {
        if (p == nullptr)
        {
            return;
        }

        Node* pNode  = static_cast<Node*>(p);
        pNode->pNext = m_pFreeBlock;
        m_pFreeBlock = pNode;
    }

    /// @brief 모든 할당된 메모리 해제
    /// 모든 풀과 프리 블록을 해제하고 초기 상태로 되돌립니다.
    void Reset()
    {
        while (m_pPoolHead)
        {
            Node* pPrev = m_pPoolHead->pNext;
            std::free(m_pPoolHead);
            m_pPoolHead = pPrev;
        }
        m_pCurBlock  = nullptr;
        m_pEndBlock  = nullptr;
        m_pFreeBlock = nullptr;
        m_pPoolHead  = nullptr;
    }

private:
    struct Node
    {
        Node* pNext = nullptr;
    };

    Node* m_pCurBlock  = nullptr;
    Node* m_pEndBlock  = nullptr;
    Node* m_pFreeBlock = nullptr;
    Node* m_pPoolHead  = nullptr;

    uint32_t m_bytePerBlock  = 0;
    uint32_t m_blocksPerPool = 0;
};

/// @brief 오브젝트 풀
/// 객체 지향적인 생성과 해제를 지원하는 MemoryPool의 얇은 래퍼
template<typename T, size_t BlocksPerPool>
class ObjectPool
{
public:
    /// @brief 생성자 및 소멸자
    ObjectPool()  = default;
    ~ObjectPool() = default;

    /// @brief 복사 및 이동 금지
    ObjectPool(const ObjectPool&)            = delete;
    ObjectPool(ObjectPool&&)                 = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool& operator=(ObjectPool&&)      = delete;

    /// @brief 객체 생성
    template<typename... Args>
    T* New(Args&&... args)
    {
        void* pMem = m_memoryPool.Alloc();
        return std::construct_at(static_cast<T*>(pMem), std::forward<Args>(args)...);
    }

    /// @brief 객체 삭제
    /// nullptr에 안전하나 디버그 모드에서는 assertion
    void Delete(T* pObj)
    {
        if (pObj)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy_at(pObj);
            }
            m_memoryPool.Free(pObj);
        }
        else
        {
            JAM_ASSERT(0, "ObjectPool::Delete - nullptr 전달됨");
        }
    }

private:
    MemoryPool m_memoryPool { sizeof(T), BlocksPerPool };
};

}   // namespace jam