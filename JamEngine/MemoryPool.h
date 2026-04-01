#pragma once

namespace Jam
{

class MemoryPool
{
public:
    explicit MemoryPool(SizeT _bytePerBlock, SizeT _blocksPerPool);
    ~MemoryPool();

    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& _pool) noexcept;
    MemoryPool& operator=(MemoryPool&& _pool) noexcept;

    [[nodiscard]] void* Alloc();
    void                Free(void* p);
    void                Reset();

private:
    struct Node
    {
        Node* pNext = nullptr;
    };

    Node* m_pCurBlock  = nullptr;
    Node* m_pEndBlock  = nullptr;
    Node* m_pFreeBlock = nullptr;
    Node* m_pPoolHead  = nullptr;

    SizeT m_bytePerBlock     = 0;
    SizeT m_numBlocksPerPool = 0;
};

template<typename T>
class ObjectPool
{
public:
    explicit ObjectPool(const SizeT _numBlocksPerPool = 2048)
        : m_pool(sizeof(T), _numBlocksPerPool)
    {
    }

    ~ObjectPool() = default;

    ObjectPool(const ObjectPool&)            = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&&)            = default;
    ObjectPool& operator=(ObjectPool&&) = default;

    template<typename... Args>
    T* New(Args&&... args)
    {
        void* pMem = m_pool.Alloc();
        return std::construct_at(static_cast<T*>(pMem), std::forward<Args>(args)...);
    }

    void Delete(T* pObj)
    {
        if (pObj)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy_at(pObj);
            }
            m_pool.Free(pObj);
        }
        else
        {
            JAM_UNREACHABLE;
        }
    }

private:
    MemoryPool m_pool;
};

}   // namespace Jam