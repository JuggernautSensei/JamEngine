#pragma once
#include "HandleAllocator.h"

namespace jam
{

template<typename T>
    requires std::is_default_constructible_v<T>
class ResourcePool
{
    template<bool IsConst>
    class Iterator
    {
    public:
        using PoolT           = std::conditional_t<IsConst, const ResourcePool, ResourcePool>;
        using HandleIteratorT = std::conditional_t<IsConst, typename HandleAllocator<T>::ConstIteratorT, typename HandleAllocator<T>::IteratorT>;
        using HandleT         = Handle<T>;
        using ResourceT       = std::conditional_t<IsConst, const T, T>;

        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::pair<HandleT, ResourceT&>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        Iterator() = default;

        explicit Iterator(PoolT* _pPool) noexcept
            : Iterator(_pPool, _pPool->m_handleAlloc.begin())
        {
        }

        explicit Iterator(PoolT* _pPool, HandleIteratorT _handleIter) noexcept
            : m_pPool(_pPool)
            , m_handleIter(_handleIter)
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
        }

        Iterator& operator++() noexcept
        {
            ++m_handleIter;
            return *this;
        }

        Iterator operator++(int) noexcept
        {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        JAM_NODISCARD value_type operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            HandleT handle = *m_handleIter;
            return { handle, m_pPool->GetResource(handle) };
        }

        JAM_NODISCARD HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            return *m_handleIter;
        }

        JAM_NODISCARD ResourceT& GetResource() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            return m_pPool->GetResource(*m_handleIter);
        }

        JAM_NODISCARD bool operator==(const Iterator& other) const noexcept
        {
            return m_pPool == other.m_pPool && m_handleIter == other.m_handleIter;
        }

        JAM_NODISCARD bool operator!=(const Iterator& other) const noexcept
        {
            return !(*this == other);
        }

    private:
        JAM_NODISCARD bool IsValid_() const noexcept
        {
            return m_pPool && m_handleIter != m_pPool->m_handleAlloc.end();
        }

        PoolT*          m_pPool      = nullptr;
        HandleIteratorT m_handleIter = {};
    };

public:
    using HandleT        = Handle<T>;
    using IteratorT      = Iterator<false>;
    using ConstIteratorT = Iterator<true>;

    ResourcePool()  = default;
    ~ResourcePool() = default;

    ResourcePool(const ResourcePool&)            = delete;
    ResourcePool(ResourcePool&&)                 = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool& operator=(ResourcePool&&)      = delete;

    JAM_NODISCARD HandleT Alloc()
    {
        HandleT handle = m_handleAlloc.Alloc();
        if (handle.index >= m_resources.size())
        {
            m_resources.resize(handle.index + 1);
        }
        ++m_itemCount;
        return handle;
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");

        T& resource = m_resources[handle.index];
        std::destroy_at(&resource);
        std::construct_at(&resource);
        m_handleAlloc.Free(handle);
        --m_itemCount;
    }

    JAM_NODISCARD T& GetResource(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");
        return m_resources[handle.index];
    }

    JAM_NODISCARD const T& GetResource(const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");
        return m_resources[handle.index];
    }

    JAM_NODISCARD const T& operator[](const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");
        return m_resources[handle.index];
    }

    JAM_NODISCARD T& operator[](const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");
        return m_resources[handle.index];
    }

    JAM_NODISCARD bool IsValid(const HandleT handle) const noexcept
    {
        return m_handleAlloc.IsValid(handle);
    }

    JAM_NODISCARD UInt32 GetCount() const noexcept
    {
        return m_itemCount;
    }

    void Clear()
    {
        m_resources.clear();
        m_handleAlloc.Clear();
        m_itemCount = 0;
    }

    JAM_NODISCARD IteratorT begin() noexcept
    {
        return IteratorT(this);
    }

    JAM_NODISCARD IteratorT end() noexcept
    {
        return IteratorT(this, m_handleAlloc.end());
    }

    JAM_NODISCARD ConstIteratorT begin() const noexcept
    {
        return ConstIteratorT(this);
    }

    JAM_NODISCARD ConstIteratorT end() const noexcept
    {
        return ConstIteratorT(this, m_handleAlloc.end());
    }

    JAM_NODISCARD ConstIteratorT cbegin() const noexcept
    {
        return ConstIteratorT(this);
    }

    JAM_NODISCARD ConstIteratorT cend() const noexcept
    {
        return ConstIteratorT(this, m_handleAlloc.end());
    }

private:
    HandleAllocator<T> m_handleAlloc = {};
    std::vector<T>     m_resources   = {};
    UInt32             m_itemCount   = 0;
};

}   // namespace jam