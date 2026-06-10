#pragma once

namespace Jam
{

struct NullOptionalT
{
    struct Tag
    {
    };

    constexpr explicit NullOptionalT(Tag) {}
};
constexpr NullOptionalT kNullopt { NullOptionalT::Tag() };

template<typename T>
class Optional;

template<typename T>
class Optional
{
    struct DummyT
    {
        constexpr explicit DummyT() {}
    };

public:
    constexpr Optional()
        : m_dummy()
        , m_bHasValue(false)
    {
    }

    constexpr ~Optional()
    {
        Reset();
    }

    template<typename... TArgs>
    constexpr explicit Optional(TArgs&&... _args)
        : m_bHasValue(true)
    {
        std::construct_at(&m_value, std::forward<TArgs>(_args)...);
    }

    /* implicit */ constexpr Optional(const T& _value)
        : m_bHasValue(true)
    {
        std::construct_at(&m_value, _value);
    }

    /* implicit */ constexpr Optional(T&& _value)
        : m_bHasValue(true)
    {
        std::construct_at(&m_value, std::move(_value));
    }

    /* implicit */ constexpr Optional(const NullOptionalT&)
        : m_bHasValue(false)
    {
        std::construct_at(&m_dummy, DummyT {});
    }

    /* implicit */ constexpr Optional(NullOptionalT&&)
        : m_bHasValue(false)
    {
        std::construct_at(&m_dummy, DummyT {});
    }

    constexpr Optional(Optional&& _other) noexcept
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, std::move(_other.m_value));
        }
        else
        {
            std::construct_at(&m_dummy, DummyT {});
        }
        _other.Reset();
    }

    constexpr Optional& operator=(Optional&& _other) noexcept
    {
        if (this != &_other)
        {
            Reset();
            m_bHasValue = _other.m_bHasValue;
            if (m_bHasValue)
            {
                std::construct_at(&m_value, std::move(_other.m_value));
            }
            else
            {
                std::construct_at(&m_dummy, DummyT {});
            }
            _other.Reset();
        }
        return *this;
    }

    constexpr Optional(const Optional& _other)
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, _other.m_value);
        }
        else
        {
            std::construct_at(&m_dummy, DummyT {});
        }
    }

    constexpr Optional& operator=(const Optional& _other)
    {
        if (this != &_other)
        {
            Reset();
            m_bHasValue = _other.m_bHasValue;
            if (m_bHasValue)
            {
                std::construct_at(&m_value, _other.m_value);
            }
            else
            {
                std::construct_at(&m_dummy, DummyT {});
            }
        }
        return *this;
    }

    constexpr Optional& operator=(const NullOptionalT&)
    {
        Reset();
        return *this;
    }

    constexpr Optional& operator=(NullOptionalT&&)
    {
        Reset();
        return *this;
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    /* implicit */ constexpr Optional(Optional<U>&& _other) noexcept(std::is_nothrow_constructible_v<T, U&&>)
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, std::move(_other.m_value));
        }
        else
        {
            std::construct_at(&m_dummy, DummyT {});
        }

        _other.Reset();
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    /* implicit */ constexpr Optional(const Optional<U>& _other)
        : m_bHasValue(_other.HasValue())
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, _other.m_value);
        }
        else
        {
            std::construct_at(&m_dummy, DummyT {});
        }
    }

    [[nodiscard]] constexpr bool HasValue() const
    {
        return m_bHasValue;
    }

    explicit constexpr operator bool() const
    {
        return m_bHasValue;
    }

    [[nodiscard]] constexpr const T& GetValue() const
    {
        JAM_ASSERT(m_bHasValue, "Optional does not contain a m_value");
        return m_value;
    }

    [[nodiscard]] constexpr T& GetValue()
    {
        JAM_ASSERT(m_bHasValue, "Optional does not contain a m_value");
        return m_value;
    }

    [[nodiscard]] constexpr T Take()
    {
        JAM_ASSERT(m_bHasValue, "Optional does not contain a m_value");
        T value = std::move(m_value);
        Reset();
        return value;
    }

    constexpr const T& operator*() const
    {
        return GetValue();
    }

    constexpr T& operator*()
    {
        return GetValue();
    }

    constexpr const T* operator->() const
    {
        return &GetValue();
    }

    constexpr T* operator->()
    {
        return &GetValue();
    }

    constexpr void Reset()
    {
        if (m_bHasValue)
        {
            std::destroy_at(&m_value);
        }

        std::construct_at(&m_dummy, DummyT {});
        m_bHasValue = false;
    }

private:
    template<typename U>
    friend class Optional;

    union
    {
        DummyT m_dummy;
        T      m_value;
    };
    bool m_bHasValue = false;
};

template<typename T>
class Optional<T&>
{
public:
    constexpr Optional()
        : m_value(nullptr)
    {
    }

    constexpr ~Optional() = default;

    /* implicit */ constexpr Optional(T& _value)
        : m_value(std::addressof(_value))
    {
    }

    /* implicit */ constexpr Optional(const NullOptionalT&)
        : m_value(nullptr)
    {
    }

    /* implicit */ constexpr Optional(NullOptionalT&&)
        : m_value(nullptr)
    {
    }

    constexpr Optional(const Optional& _other)                = default;
    constexpr Optional& operator=(const Optional& _other)     = default;
    constexpr Optional(Optional&& _other) noexcept            = default;
    constexpr Optional& operator=(Optional&& _other) noexcept = default;

    constexpr Optional& operator=(const NullOptionalT&)
    {
        Reset();
        return *this;
    }

    constexpr Optional& operator=(NullOptionalT&&)
    {
        Reset();
        return *this;
    }

    [[nodiscard]] constexpr bool HasValue() const
    {
        return m_value != nullptr;
    }

    explicit constexpr operator bool() const
    {
        return m_value != nullptr;
    }

    [[nodiscard]] constexpr T& GetValue() const
    {
        JAM_ASSERT(m_value != nullptr, "Optional does not contain a m_value");
        return *m_value;
    }

    [[nodiscard]] constexpr T& Take()
    {
        JAM_ASSERT(m_value != nullptr, "Optional does not contain a m_value");
        T* pValue = m_value;
        Reset();
        return *pValue;
    }

    constexpr T& operator*() const
    {
        return GetValue();
    }

    constexpr T* operator->() const
    {
        return m_value;
    }

    constexpr void Reset()
    {
        m_value = nullptr;
    }

private:
    T* m_value = nullptr;
};

}   // namespace Jam
