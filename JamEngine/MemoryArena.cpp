#include "pch.h"
#include "MemoryArena.h"

#include "Allocator.h"

namespace Jam
{

MemoryArena::MemoryArena(
    const SizeT _byteWidthPerArena)
    : m_byteWidthPerArena(_byteWidthPerArena)
{
    JAM_ASSERT(_byteWidthPerArena > 0, "아레나의 크기는 0보다 커야 합니다.");
}

MemoryArena::~MemoryArena()
{
    Reset();
}

void* MemoryArena::Alloc(const SizeT _byteWidth)
{
    const SizeT byteWidth = AlignUp<SizeT>(_byteWidth, alignof(std::max_align_t));
    JAM_ASSERT(std::cmp_less_equal(byteWidth, m_byteWidthPerArena), "한 아레나에 할당할 수 있는 최대 크기를 초과했습니다.");

    if (m_pCur == nullptr)   // 아레나 초기화 필요
    {
        void* pMem            = Malloc(m_byteWidthPerArena + sizeof(Head));
        m_pCurHead            = static_cast<Head*>(pMem);
        m_pCurHead->pPrevHead = nullptr;
        m_pCur                = static_cast<Byte*>(pMem) + sizeof(Head);
        m_pEnd                = m_pCur + m_byteWidthPerArena;
    }

    if (m_pCur + byteWidth >= m_pEnd)
    {
        void* pMem          = Malloc(m_byteWidthPerArena + sizeof(Head));
        Head* pNewHead      = static_cast<Head*>(pMem);
        pNewHead->pPrevHead = m_pCurHead;
        m_pCurHead          = pNewHead;
        m_pCur              = static_cast<Byte*>(pMem) + sizeof(Head);
        m_pEnd              = m_pCur + m_byteWidthPerArena;
    }

    void* pRet = m_pCur;
    m_pCur     = m_pCur + byteWidth;
    return pRet;
}

void MemoryArena::Reset()
{
    while (m_pCurHead)
    {
        Head* pPrevHead = m_pCurHead->pPrevHead;
        Free(m_pCurHead);
        m_pCurHead = pPrevHead;
    }
    m_pCur = nullptr;
    m_pEnd = nullptr;
}

}   // namespace Jam