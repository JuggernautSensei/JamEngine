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

        [[nodiscard]] value_type operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            HandleT handle = *m_handleIter;
            return { handle, m_pPool->GetResource(handle) };
        }

        [[nodiscard]] HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            return *m_handleIter;
        }

        [[nodiscard]] ResourceT& GetResource() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator not initialized or out of bounds");
            return m_pPool->GetResource(*m_handleIter);
        }

        [[nodiscard]] bool operator==(const Iterator& other) const noexcept
        {
            return m_pPool == other.m_pPool && m_handleIter == other.m_handleIter;
        }

        [[nodiscard]] bool operator!=(const Iterator& other) const noexcept
        {
            return !(*this == other);
        }

    private:
        [[nodiscard]] bool IsValid_() const noexcept
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

    [[nodiscard]] HandleT Alloc()
    {
        HandleT handle = m_handleAlloc.Alloc();
        if (handle.index >= m_resources.size())
        {
            m_resources.resize(handle.index + 1);
        }
        ++m_numItems;
        return handle;
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");

        T& resource = m_resources[handle.index];
        std::destroy_at(&resource);
        std::construct_at(&resource);
        m_handleAlloc.Free(handle);
        --m_numItems;
    }

    [[nodiscard]] T& GetResource(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. index = {}, token = {}", handle.index, handle.token);
        return m_resources[handle.index];
    }

    [[nodiscard]] const T& GetResource(const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. index = {}, token = {}", handle.index, handle.token);
        return m_resources[handle.index];
    }

    [[nodiscard]] const T& operator[](const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. index = {}, token = {}", handle.index, handle.token);
        return m_resources[handle.index];
    }

    [[nodiscard]] T& operator[](const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. index = {}, token = {}", handle.index, handle.token);
        return m_resources[handle.index];
    }

    [[nodiscard]] bool IsValid(const HandleT handle) const noexcept
    {
        return m_handleAlloc.IsValid(handle);
    }

    [[nodiscard]] UInt32 GetCount() const noexcept
    {
        return m_numItems;
    }

    void Clear()
    {
        m_resources.clear();
        m_handleAlloc.Clear();
        m_numItems = 0;
    }

    [[nodiscard]] IteratorT begin() noexcept
    {
        return IteratorT(this);
    }

    [[nodiscard]] IteratorT end() noexcept
    {
        return IteratorT(this, m_handleAlloc.end());
    }

    [[nodiscard]] ConstIteratorT begin() const noexcept
    {
        return ConstIteratorT(this);
    }

    [[nodiscard]] ConstIteratorT end() const noexcept
    {
        return ConstIteratorT(this, m_handleAlloc.end());
    }

    [[nodiscard]] ConstIteratorT cbegin() const noexcept
    {
        return ConstIteratorT(this);
    }

    [[nodiscard]] ConstIteratorT cend() const noexcept
    {
        return ConstIteratorT(this, m_handleAlloc.end());
    }

private:
    HandleAllocator<T> m_handleAlloc = {};
    Vector<T>     m_resources   = {};
    UInt32             m_numItems    = 0;
};

}   // namespace jam