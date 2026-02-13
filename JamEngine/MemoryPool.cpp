#include "pch.h"

#include "MemoryPool.h"

#include "MemoryAllocator.h"

namespace Jam
{

MemoryPool::MemoryPool(
    const USizeT _bytePerBlock,
    const USizeT _blocksPerPool)
    : m_bytePerBlock(_bytePerBlock >= sizeof(Node) ? _bytePerBlock : sizeof(Node))
    , m_blocksPerPool(_blocksPerPool)
{
    m_bytePerBlock = AlignUp(m_bytePerBlock);
}

MemoryPool::~MemoryPool()
{
    Reset();
}

MemoryPool::MemoryPool(MemoryPool&& _pool) noexcept
{
    m_pCurBlock  = _pool.m_pCurBlock;
    m_pEndBlock  = _pool.m_pEndBlock;
    m_pFreeBlock = _pool.m_pFreeBlock;
    m_pPoolHead  = _pool.m_pPoolHead;

    m_bytePerBlock  = _pool.m_bytePerBlock;
    m_blocksPerPool = _pool.m_blocksPerPool;
}

MemoryPool& MemoryPool::operator=(MemoryPool&& _pool) noexcept
{
    if (this != &_pool)
    {
        Reset();

        m_pCurBlock  = _pool.m_pCurBlock;
        m_pEndBlock  = _pool.m_pEndBlock;
        m_pFreeBlock = _pool.m_pFreeBlock;
        m_pPoolHead  = _pool.m_pPoolHead;

        m_bytePerBlock  = _pool.m_bytePerBlock;
        m_blocksPerPool = _pool.m_blocksPerPool;
    }
    return *this;
}

void* MemoryPool::Alloc()
{
    if (m_pCurBlock == nullptr)   // 풀 초기화 필요
    {
        Byte* pMem         = static_cast<Byte*>(Malloc(sizeof(Node) + m_bytePerBlock * m_blocksPerPool));
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
            Byte* pMem  = static_cast<Byte*>(Malloc(sizeof(Node) + m_bytePerBlock * m_blocksPerPool));
            m_pCurBlock = reinterpret_cast<Node*>(pMem + sizeof(Node));
            m_pEndBlock = reinterpret_cast<Node*>(pMem + sizeof(Node) + m_blocksPerPool * m_bytePerBlock);

            // 새 풀을 링크드 리스트에 추가
            Node* pNewPoolHead  = reinterpret_cast<Node*>(pMem);
            pNewPoolHead->pNext = m_pPoolHead;
            m_pPoolHead         = pNewPoolHead;
        }

        // 현재 블록에서 새 블록 할당
        Node* pRet  = m_pCurBlock;
        m_pCurBlock = reinterpret_cast<Node*>(reinterpret_cast<Byte*>(m_pCurBlock) + m_bytePerBlock);
        pRet->pNext = nullptr;
        return pRet;
    }
}

void MemoryPool::Free(void* p)
{
    if (p == nullptr)
    {
        return;
    }

    Node* pNode  = static_cast<Node*>(p);
    pNode->pNext = m_pFreeBlock;
    m_pFreeBlock = pNode;
}

void MemoryPool::Reset()
{
    while (m_pPoolHead)
    {
        Node* pPrev = m_pPoolHead->pNext;
        Free(m_pPoolHead);
        m_pPoolHead = pPrev;
    }
    m_pCurBlock  = nullptr;
    m_pEndBlock  = nullptr;
    m_pFreeBlock = nullptr;
    m_pPoolHead  = nullptr;
}

}   // namespace Jam
