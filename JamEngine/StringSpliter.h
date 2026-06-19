#pragma once

namespace Jam
{

// use like this:
// String str = "a, b, c, d, e";
// StringSpliter spliter { str, ',' };
//
// for (; !spliter.IsDone(); spliter.Next())
// {
//   StringView token = spliter.Get();
//   token = token.Trimmed();
//   std::cout << token << std::endl;
// }
//
// for (StringView token : spliter)
// {
//    token = token.Trimmed();
//    std::cout << token << std::endl;
// }

template<IsCharT TChar>
class StringSpliterBase
{
public:
    using StringViewT = BasicStringView<TChar>;

    class Iterator
    {
    public:
        Iterator() = default;

        explicit Iterator(StringSpliterBase* _pOwner)
            : m_pOwner(_pOwner)
        {
        }

        StringViewT operator*() const
        {
            JAM_ASSERT(m_pOwner, "Iterator not initialized");
            return m_pOwner->Get();
        }

        Iterator& operator++()
        {
            JAM_ASSERT(m_pOwner, "Iterator not initialized");

            m_pOwner->Next();
            if (m_pOwner->IsDone())
            {
                m_pOwner = nullptr;
            }
            return *this;
        }

        bool operator==(const Iterator& _other) const
        {
            return m_pOwner == _other.m_pOwner;
        }

        bool operator!=(const Iterator& _other) const
        {
            return m_pOwner != _other.m_pOwner;
        }

    private:
        StringSpliterBase* m_pOwner = nullptr;
    };

public:
    explicit StringSpliterBase(
        const StringViewT _str,
        const TChar       _delimiter,
        const bool        _bSkipEmpty = true)
        : m_remain(_str)
        , m_delimiter(_delimiter)
        , m_bSkipEmpty(_bSkipEmpty)
    {
        Next();
    }

    [[nodiscard]] StringViewT Get() const
    {
        return m_ret;
    }

    void Next()
    {
        while (!m_bDone)
        {
            if (m_remain.IsEmpty())
            {
                m_ret   = {};
                m_bDone = true;
                return;
            }

            const Optional<SizeT> pos = m_remain.Find(m_delimiter);
            if (pos)
            {
                m_ret    = m_remain.SubView(0, pos.GetValue());
                m_remain = m_remain.SubView(pos.GetValue() + 1);
            }
            else
            {
                m_ret    = m_remain;
                m_remain = {};
            }

            if (!m_bSkipEmpty || !m_ret.IsEmpty())
            {
                return;
            }
        }
    }

    bool IsDone() const
    {
        return m_bDone;
    }

    [[nodiscard]] Iterator Begin()
    {
        return Iterator { this };
    }

    [[nodiscard]] Iterator End()
    {
        return Iterator { nullptr };
    }

public:   // for STL
    using iterator = Iterator;

    iterator begin()
    {
        return Begin();
    }

    iterator end()
    {
        return End();
    }

private:
    StringSpliterBase() = default;

    StringViewT m_remain     = {};
    StringViewT m_ret        = {};
    TChar       m_delimiter  = {};
    bool        m_bSkipEmpty = true;
    bool        m_bDone      = false;
};

using StringSpliter  = StringSpliterBase<char>;
using WStringSpliter = StringSpliterBase<wchar_t>;

}   // namespace Jam