#pragma once
#include "HandleAllocator.h"

namespace Jam
{

template<typename T>
    requires std::is_default_constructible_v<T>
class ResourcePool
{
public:
    using HandleT = Handle<T>;

    template<bool IsConst>
    class IteratorBase
    {
    public:
        using PoolT          = std::conditional_t<IsConst, const ResourcePool, ResourcePool>;
        using HandleT        = Handle<T>;
        using ResourceT      = std::conditional_t<IsConst, const T, T>;
        using HandleIterator = std::conditional_t<IsConst, typename HandleAllocator<T>::ConstIterator, typename HandleAllocator<T>::Iterator>;

        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::pair<HandleT, ResourceT&>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        IteratorBase() = default;

        explicit IteratorBase(PoolT* _pPool) noexcept
            : IteratorBase(_pPool, _pPool->m_handleAlloc.begin())
        {
        }

        explicit IteratorBase(PoolT* _pPool, HandleIterator _handleIter) noexcept
            : m_pPool(_pPool)
            , m_handleIter(_handleIter)
        {
        }

        IteratorBase& operator++() noexcept
        {
            ++m_handleIter;
            return *this;
        }

        IteratorBase operator++(int) noexcept
        {
            IteratorBase temp = *this;
            ++(*this);
            return temp;
        }

        [[nodiscard]] value_type operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "IteratorBase not initialized or out of bounds");
            HandleT handle = *m_handleIter;
            return { handle, m_pPool->GetResource(handle) };
        }

        [[nodiscard]] HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "IteratorBase not initialized or out of bounds");
            return *m_handleIter;
        }

        [[nodiscard]] ResourceT& GetResource() const noexcept
        {
            JAM_ASSERT(IsValid_(), "IteratorBase not initialized or out of bounds");
            return m_pPool->GetResource(*m_handleIter);
        }

        [[nodiscard]] bool operator==(const IteratorBase& other) const noexcept
        {
            return m_pPool == other.m_pPool && m_handleIter == other.m_handleIter;
        }

        [[nodiscard]] bool operator!=(const IteratorBase& other) const noexcept
        {
            return !(*this == other);
        }

    private:
        [[nodiscard]] bool IsValid_() const noexcept
        {
            return m_pPool && m_handleIter != m_pPool->m_handleAlloc.end();
        }

        PoolT*         m_pPool      = nullptr;
        HandleIterator m_handleIter = {};
    };

    using Iterator      = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    ResourcePool()  = default;
    ~ResourcePool() = default;

    ResourcePool(const ResourcePool&)            = delete;
    ResourcePool(ResourcePool&&)                 = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool& operator=(ResourcePool&&)      = delete;

    [[nodiscard]] HandleT Alloc()
    {
        HandleT handle = m_handleAlloc.Alloc();
        if (std::cmp_greater_equal(handle.GetIndex(), m_resources.size()))
        {
            m_resources.resize(handle.GetIndex() + 1);
        }
        ++m_numItems;
        return handle;
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");

        T& resource = m_resources[handle.GetIndex()];
        std::destroy_at(&resource);
        std::construct_at(&resource);
        m_handleAlloc.Free(handle);
        --m_numItems;
    }

    [[nodiscard]] T& GetResource(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. pos = {}, token = {}", handle.GetIndex(), handle.GetToken());
        return m_resources[handle.GetIndex()];
    }

    [[nodiscard]] const T& GetResource(const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. pos = {}, token = {}", handle.GetIndex(), handle.GetToken());
        return m_resources[handle.GetIndex()];
    }

    [[nodiscard]] const T& operator[](const HandleT handle) const noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. pos = {}, token = {}", handle.GetIndex(), handle.GetToken());
        return m_resources[handle.GetIndex()];
    }

    [[nodiscard]] T& operator[](const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "invalid handle. pos = {}, token = {}", handle.GetIndex(), handle.GetToken());
        return m_resources[handle.GetIndex()];
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

    [[nodiscard]] Iterator begin() noexcept
    {
        return Iterator(this);
    }

    [[nodiscard]] Iterator end() noexcept
    {
        return Iterator(this, m_handleAlloc.end());
    }

    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return ConstIterator(this);
    }

    [[nodiscard]] ConstIterator end() const noexcept
    {
        return ConstIterator(this, m_handleAlloc.end());
    }

    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return ConstIterator(this);
    }

    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return ConstIterator(this, m_handleAlloc.end());
    }

private:
    HandleAllocator<T> m_handleAlloc = {};
    Vector<T>          m_resources   = {};
    UInt32             m_numItems    = 0;
};

}   // namespace Jam