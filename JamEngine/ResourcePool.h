#pragma once
#include "Graphics.h"
#include "HandleAllocator.h"

namespace Jam
{

template<typename T>
    requires std::is_default_constructible_v<T>
class ResourcePool
{
public:
    using HandleT = Handle<T>;

public:
    class ConstIterator
    {
    public:
        using HandleIteratorT = HandleAllocator<T>::Iterator;

        ConstIterator() = default;

        explicit ConstIterator(ResourcePool* _pPool, HandleIteratorT _handleIter) noexcept
            : m_pPool(_pPool)
            , m_handleIter(_handleIter)
        {
        }

        ConstIterator& operator++() noexcept
        {
            ++m_handleIter;
            return *this;
        }

        [[nodiscard]] std::pair<HandleT, const T&> operator*() const noexcept
        {
            JAM_ASSERT(m_pPool, "Iterator not initialized");

            const HandleT handle = *m_handleIter;
            return { handle, m_pPool->GetResource(handle) };
        }

        [[nodiscard]] bool operator==(const ConstIterator& other) const noexcept
        {
            return m_pPool == other.m_pPool && m_handleIter == other.m_handleIter;
        }

        [[nodiscard]] bool operator!=(const ConstIterator& other) const noexcept
        {
            return !(*this == other);
        }

    protected:
        ResourcePool*   m_pPool      = nullptr;
        HandleIteratorT m_handleIter = {};
    };

    class Iterator : public ConstIterator
    {
        using Base = ConstIterator;

    public:
        [[nodiscard]] std::pair<HandleT, T&> operator*() noexcept
        {
            JAM_ASSERT(this->m_pPool, "Iterator not initialized");
            const HandleT handle = Base::m_handleIter;
            return { handle, Base::m_pPool->GetResource(handle) };
        }
    };

public:
    ResourcePool()  = default;
    ~ResourcePool() = default;

    ResourcePool(const ResourcePool&)            = delete;
    ResourcePool(ResourcePool&&)                 = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool& operator=(ResourcePool&&)      = delete;

    [[nodiscard]] HandleT Alloc()
    {
        HandleT handle = m_handleAlloc.Alloc();
        if (handle.GetIndex() >= m_resources.size())
        {
            m_resources.resize(handle.GetIndex() + 1);
        }
        ++m_numResources;
        return handle;
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(m_handleAlloc.IsValid(handle), "Invalid handle");

        T& resource = m_resources[handle.GetIndex()];
        std::destroy_at(&resource);
        std::construct_at(&resource);
        m_handleAlloc.Free(handle);
        --m_numResources;
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
        return m_numResources;
    }

    void Clear()
    {
        m_resources.clear();
        m_handleAlloc.Clear();
        m_numResources = 0;
    }

    [[nodiscard]] Iterator Begin() noexcept
    {
        return { this, m_handleAlloc.Begin() };
    }

    [[nodiscard]] Iterator End() noexcept
    {
        return { this, m_handleAlloc.End() };
    }

    [[nodiscard]] ConstIterator Begin() const noexcept
    {
        return ConstIterator { this, m_handleAlloc.Begin() };
    }

    [[nodiscard]] ConstIterator End() const noexcept
    {
        return ConstIterator { this, m_handleAlloc.End() };
    }

public:   // for STL
    [[nodiscard]] Iterator begin() noexcept
    {
        return Begin();
    }

    [[nodiscard]] Iterator end() noexcept
    {
        return End();
    }

    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return Begin();
    }

    [[nodiscard]] ConstIterator end() const noexcept
    {
        return End();
    }

private:
    HandleAllocator<T> m_handleAlloc  = {};
    Vector<T>          m_resources    = {};
    Int32              m_numResources = 0;
};

}   // namespace Jam