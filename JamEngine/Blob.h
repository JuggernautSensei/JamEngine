#pragma once

#include <memory>
#include <utility>

namespace Jam
{

template<typename Deleter = std::default_delete<Byte[]>>
class BasicBlob
{
public:
    using ValueT   = Byte;
    using DeleterT = Deleter;

    BasicBlob() noexcept
        : m_ptr(nullptr)
        , m_byteWidth(0)
        , m_deleter()
    {
    }

    /* implicit */ BasicBlob(std::nullptr_t)
        : BasicBlob()
    {
    }

    // 포인터 주입. 소유권이 이전된다. 새로운 Blob의 할당은 AllocBlob() 함수를 이용할 것.
    /* implicit */ BasicBlob(Byte* _ptr, const SizeT _byteWidth) noexcept
        : m_ptr(_ptr)
        , m_byteWidth(_byteWidth)
        , m_deleter()
    {
    }

    // 포인터 주입. 소유권이 이전된다. 새로운 Blob의 할당은 AllocBlob() 함수를 이용할 것.
    /* implicit */ BasicBlob(Byte* _ptr, const SizeT _byteWidth, Deleter&& _deleter) noexcept
        : m_ptr(_ptr)
        , m_byteWidth(_byteWidth)
        , m_deleter(std::move(_deleter))
    {
    }

    // 포인터 주입. 소유권이 이전된다. 새로운 Blob의 할당은 AllocBlob() 함수를 이용할 것.
    /* implicit */ BasicBlob(Byte* _ptr, const SizeT _byteWidth, const Deleter& _deleter) noexcept
        : m_ptr(_ptr)
        , m_byteWidth(_byteWidth)
        , m_deleter(_deleter)
    {
    }

    ~BasicBlob()
    {
        Free();
    }

    BasicBlob(const BasicBlob&) = delete;

    BasicBlob(BasicBlob&& _other) noexcept
        : m_ptr(_other.m_ptr)
        , m_byteWidth(_other.m_byteWidth)
        , m_deleter(std::move(_other.m_deleter))
    {
        _other.m_ptr       = nullptr;
        _other.m_byteWidth = 0;
    }

    template<typename E>
        requires(std::is_convertible_v<E, Deleter>)
    /* implicit */ BasicBlob(BasicBlob<E>&& _other) noexcept
        : m_ptr(_other.m_ptr)
        , m_byteWidth(_other.m_byteWidth)
        , m_deleter(std::forward<E>(_other.m_deleter))
    {
        _other.m_ptr       = nullptr;
        _other.m_byteWidth = 0;
    }

    BasicBlob& operator=(const BasicBlob&) = delete;

    BasicBlob& operator=(BasicBlob&& _other) noexcept
    {
        if (this != &_other)
        {
            Free();
            m_ptr              = _other.m_ptr;
            m_byteWidth        = _other.m_byteWidth;
            m_deleter          = std::move(_other.m_deleter);
            _other.m_ptr       = nullptr;
            _other.m_byteWidth = 0;
        }
        return *this;
    }

    template<typename E>
        requires(std::is_convertible_v<E, Deleter>)
    BasicBlob& operator=(BasicBlob<E>&& _other) noexcept
    {
        Free();
        m_ptr       = _other.m_ptr;
        m_byteWidth = _other.m_byteWidth;
        m_deleter   = std::forward<E>(_other.m_deleter);

        _other.m_ptr       = nullptr;
        _other.m_byteWidth = 0;
        return *this;
    }

    BasicBlob& operator=(std::nullptr_t)
    {
        Free();
        return *this;
    }

    [[nodiscard]] Byte* GetPtr() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] SizeT GetByteWidth() const noexcept
    {
        return m_byteWidth;
    }

    [[nodiscard]] Byte* Release() noexcept
    {
        Byte* p     = m_ptr;
        m_ptr       = nullptr;
        m_byteWidth = 0;
        return p;
    }

    void Free() noexcept
    {
        if (m_ptr)
        {
            m_deleter(m_ptr);
            m_ptr       = nullptr;
            m_byteWidth = 0;
        }
    }

    [[nodiscard]] Deleter& GetDeleter() noexcept
    {
        return m_deleter;
    }

    [[nodiscard]] const Deleter& GetDeleter() const noexcept
    {
        return m_deleter;
    }

    /* implicit */ operator bool() const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return m_ptr == nullptr;
    }

    [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept
    {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool operator==(const BasicBlob& _other) const noexcept
    {
        return m_ptr == _other.m_ptr;
    }

    [[nodiscard]] bool operator!=(const BasicBlob& _other) const noexcept
    {
        return m_ptr != _other.m_ptr;
    }

private:
    Byte*                         m_ptr       = nullptr;
    SizeT                         m_byteWidth = 0;
    [[no_unique_address]] Deleter m_deleter   = Deleter {};

    template<typename E>
    friend class BasicBlob;
};

using Blob = BasicBlob<>;

[[nodiscard]] Blob AllocBlob(SizeT _byteWidth);

}   // namespace Jam