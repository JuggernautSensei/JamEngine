// ReSharper disable CppClangTidyClangDiagnosticFloatEqual
#pragma once
#include "StringBase.h"

namespace Jam
{

template<IsCharT TChar>
class BasicString : public StringBase<BasicString<TChar>, TChar>
{
public:
    using BaseT       = StringBase<BasicString, TChar>;
    using CharTraits  = CharTraits<TChar>;
    using CharT       = CharTraits::CharT;
    using StringViewT = BasicStringView<CharT>;
    using Iter        = CharT*;
    using ConstIter   = const CharT*;

    template<typename... Args>
    using FormatStringT = CharTraits::template FormatStringT<Args...>;

    using BaseT::Append;
    using BaseT::Insert;
    using BaseT::Set;

private:
    static constexpr Byte  kHeapEnabledBit = 0xFF;
    static constexpr SizeT kGrowthFactor   = 2;
    static constexpr SizeT kLayoutSize     = 32;
    static constexpr SizeT kSSOBufSize     = (kLayoutSize - 1) / sizeof(TChar);
    static constexpr SizeT kSSOCap         = kSSOBufSize - 1;

    union RawData
    {
        struct
        {
            TChar sso[kSSOBufSize];
        };

        struct
        {
            TChar* heap;
            SizeT  heapLen;
            SizeT  heapCap;
        };

        struct
        {
            Byte padding[kLayoutSize - 1];
            Byte bit;
        };
    };

    static_assert(sizeof(RawData) == 32);
    static_assert(offsetof(RawData, RawData::bit) == 31);

public:
    BasicString()
    {
        ResizeSSO_(0);
    }

    ~BasicString()
    {
        Free_();
    }

    /* implicit */ BasicString(
        const StringViewT _str)
    {
        ResizeSSO_(0);
        Set(_str);
    }

    /* implicit */ BasicString(
        const CharT* _str)
    {
        ResizeSSO_(0);
        Set(_str);
    }

    /* implicit */ BasicString(
        const CharT* _str,
        const SizeT  _n)
    {
        ResizeSSO_(0);
        Set({ _str, _n });
    }

    /* implicit */ BasicString(
        const SizeT _count,
        const CharT _c)
    {
        ResizeSSO_(0);
        Resize(_count, _c);
    }

    /* implicit */ BasicString(
        const std::initializer_list<TChar> _list)
    {
        ResizeSSO_(0);
        Set({ _list.begin(), static_cast<SizeT>(_list.size()) });
    }

    BasicString(
        const BasicString& _other)
        : BasicString(_other.ToStringView())
    {
    }

    BasicString(
        BasicString&& _other) noexcept
        : m_str(_other.m_str)
    {
        _other.ResizeSSO_(0);
    }

    BasicString& operator=(
        const BasicString& _other)
    {
        if (this != &_other)
        {
            Set(_other.ToStringView());
        }
        return *this;
    }

    BasicString& operator=(
        BasicString&& _other) noexcept
    {
        if (this != &_other)
        {
            Free_();
            m_str = _other.m_str;
            _other.ResizeSSO_(0);
        }
        return *this;
    }

    BasicString& operator=(
        const StringViewT _other)
    {
        Set(_other);
        return *this;
    }

    BasicString& operator=(
        const TChar _c)
    {
        Set(_c);
        return *this;
    }

    BasicString& operator=(
        const CharT* _ptr)
    {
        Set(_ptr);
        return *this;
    }

    BasicString& operator=(
        const std::initializer_list<TChar> _list)
    {
        Set({ _list.begin(), static_cast<SizeT>(_list.size()) });
        return *this;
    }

    void Set(
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();

        if (IsHeap_())
        {
            GrowHeapIfNeed_(len);
            SetToHeap_(_str);
        }
        else if (len > kSSOCap)   // change to heap
        {
            InitHeap_(len * kGrowthFactor);
            SetToHeap_(_str);
        }
        else
        {
            SetToSSO_(_str);
        }
    }

    void Append(
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();

        if (IsHeap_())
        {
            GrowHeapIfNeed_(m_str.heapLen + len);
            AppendToHeap_(_str);
        }
        else if (m_str.bit < len)   // change to heap
        {
            SSOToHeap_((GetLengthSSO_() + len) * kGrowthFactor);
            AppendToHeap_(_str);
        }
        else
        {
            AppendToSSO_(_str);
        }
    }

    void Insert(
        const SizeT       _pos,
        const StringViewT _str)
    {
        const SizeT len = _str.GetLength();
        if (IsHeap_())
        {
            GrowHeapIfNeed_(GetLength() + len);
            InsertToHeap_(_pos, _str);
        }
        else if (m_str.bit < len)   // change to heap
        {
            SSOToHeap_((GetLengthSSO_() + len) * kGrowthFactor);
            InsertToHeap_(_pos, _str);
        }
        else
        {
            InsertToSSO_(_pos, _str);
        }
    }

    void Erase(
        const SizeT _pos,
        const SizeT _count)
    {
        if (IsHeap_())
        {
            EraseAtHeap_(_pos, _count);
        }
        else
        {
            EraseAtSSO_(_pos, _count);
        }
    }

    void Clear()
    {
        if (IsHeap_())
        {
            ResizeHeap_(0);
        }
        else
        {
            ResizeSSO_(0);
        }
    }

    void Reserve(
        const SizeT _newCap)
    {
        if (IsHeap_())
        {
            if (m_str.heapCap < _newCap)
            {
                ReallocHeap_(_newCap);
            }
        }
        else
        {
            if (_newCap > kSSOCap)
            {
                SSOToHeap_(_newCap);
            }
        }
    }

    void Resize(
        const SizeT _newLen,
        const CharT _c = CharTraits::kNullChar)
    {
        Resize_<true>(_newLen, _c);
    }

    void ResizeUninitialized(
        const SizeT _newLen)
    {
        Resize_<false>(_newLen, CharTraits::kNullChar);
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return GetLength() == 0;
    }

    [[nodiscard]] SizeT GetLength() const
    {
        return IsHeap_() ? m_str.heapLen : kSSOCap - m_str.bit;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return IsHeap_() ? m_str.heapCap : kSSOCap;
    }

    [[nodiscard]] const CharT* GetPtr() const
    {
        return IsHeap_() ? m_str.heap : m_str.sso;
    }

    [[nodiscard]] CharT* GetPtr()
    {
        return IsHeap_() ? m_str.heap : m_str.sso;
    }

private:
    template<bool kbInitialize>
    void Resize_(
        const SizeT _newLen,
        const CharT _c)
    {
        const SizeT len = GetLength();

        // resize
        if (IsHeap_())
        {
            GrowHeapIfNeed_(_newLen);
            ResizeHeap_(_newLen);
        }
        else
        {
            if (_newLen > kSSOCap)
            {
                SSOToHeap_(_newLen * kGrowthFactor);
                ResizeHeap_(_newLen);
            }
            else
            {
                ResizeSSO_(_newLen);
            }
        }

        // initialize new chars if needed
        if constexpr (kbInitialize)
        {
            if (len < _newLen)
            {
                CharT* ptr = GetPtr();
                std::fill(ptr + len, ptr + _newLen, _c);
            }
        }
    }

    void Free_() const
    {
        if (IsHeap_())
        {
            Free(m_str.heap);
        }
    }

    [[nodiscard]] bool IsHeap_() const
    {
        return m_str.bit == kHeapEnabledBit;
    }

    [[nodiscard]] bool IsSSO_() const
    {
        return !IsHeap_();
    }

    void InitHeap_(
        const SizeT _cap)
    {
        m_str.heap    = static_cast<TChar*>(Malloc((_cap + 1) * sizeof(TChar)));
        m_str.heapCap = _cap;
        m_str.bit     = kHeapEnabledBit;
    }

    void GrowHeapIfNeed_(
        const SizeT _newCap)
    {
        if (m_str.heapCap < _newCap)
        {
            ReallocHeap_(_newCap * kGrowthFactor);
        }
    }

    void ReallocHeap_(
        const SizeT _newCap)
    {
        m_str.heap    = static_cast<TChar*>(Realloc(m_str.heap, (_newCap + 1) * sizeof(TChar)));
        m_str.heapCap = _newCap;
    }

    void SetToHeap_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "SetToHeap_: must be in heap mode to set heap data.");
        JAM_ASSERT(_str.GetLength() <= m_str.heapCap, "SetToHeap_: _str length exceeds heap capacity.");

        const SizeT len = _str.GetLength();
        ::memcpy(m_str.heap, _str.GetPtr(), len * sizeof(TChar));
        ResizeHeap_(len);   // for null terminator and length update
    }

    void ResizeHeap_(
        const SizeT _newLen)
    {
        JAM_ASSERT(IsHeap_(), "ResizeHeap_: must be in heap mode to resize heap data.");
        JAM_ASSERT(_newLen <= m_str.heapCap, "ResizeHeap_: _newLen exceeds heap capacity.");
        m_str.heap[_newLen] = CharTraits::kNullChar;
        m_str.heapLen       = _newLen;
    }

    void AppendToHeap_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "AppendToHeap_: must be in heap mode to append to heap.");
        JAM_ASSERT(_str.GetLength() + m_str.heapLen <= m_str.heapCap, "AppendToHeap_: combined length exceeds heap capacity.");

        ::memcpy(m_str.heap + m_str.heapLen, _str.GetPtr(), _str.GetLength() * sizeof(CharT));
        ResizeHeap_(m_str.heapLen + _str.GetLength());
    }

    void InsertToHeap_(
        const SizeT       _pos,
        const StringViewT _str)
    {
        JAM_ASSERT(IsHeap_(), "InsertToHeap_: must be in heap mode to insert to heap.");
        JAM_ASSERT(_pos <= m_str.heapLen, "InsertToHeap_: _pos exceeds string length.");

        ::memmove(m_str.heap + _pos + _str.GetLength(), m_str.heap + _pos, (m_str.heapLen - _pos) * sizeof(CharT));
        ::memcpy(m_str.heap + _pos, _str.GetPtr(), _str.GetLength() * sizeof(CharT));
        ResizeHeap_(m_str.heapLen + _str.GetLength());
    }

    void EraseAtHeap_(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(IsHeap_(), "EraseAtHeap_: must be in heap mode to erase heap data.");

        const SizeT total = _pos + _count;
        const SizeT len   = m_str.heapLen;
        JAM_ASSERT(_pos <= len, "EraseAtHeap_: _pos exceeds string length.");
        JAM_ASSERT(total <= len, "EraseAtHeap_: _pos + _count exceeds string length.");

        ::memmove(m_str.heap + _pos, m_str.heap + total, (len - total) * sizeof(CharT));
        ResizeHeap_(len - _count);
    }

    [[nodiscard]] SizeT GetLengthSSO_() const
    {
        JAM_ASSERT(IsSSO_(), "LengthOfSSO_: must be in stack mode to get stack data length.");
        return kSSOCap - m_str.bit;
    }

    void SetToSSO_(
        const StringViewT _str)
    {
        JAM_ASSERT(IsSSO_(), "SetToStack: must be in stack mode to set stack data.");
        JAM_ASSERT(_str.GetLength() <= kSSOCap, "SetToStack: _str length exceeds stack capacity.");

        const SizeT len = _str.GetLength();
        ::memcpy(m_str.sso, _str.GetPtr(), len * sizeof(TChar));
        ResizeSSO_(len);   // for null terminator and left update
    }

    void ResizeSSO_(
        const SizeT _len)
    {
        JAM_ASSERT(_len <= kSSOCap, "ResizeSSO_: _len exceeds sso capacity.");
        m_str.sso[_len] = CharTraits::kNullChar;
        m_str.bit       = static_cast<Byte>(kSSOCap - _len);
    }

    void AppendToSSO_(
        const StringViewT _str)
    {
        const SizeT len = GetLengthSSO_();

        JAM_ASSERT(IsSSO_(), "AppendToSSO_: must be in stack mode to append to stack.");
        JAM_ASSERT(_str.GetLength() + len <= kSSOCap, "AppendToSSO_: combined length exceeds stack capacity.");

        ::memcpy(m_str.sso + len, _str.GetPtr(), _str.GetLength() * sizeof(TChar));
        ResizeSSO_(len + _str.GetLength());
    }

    void InsertToSSO_(
        const SizeT       _pos,
        const StringViewT _str)
    {
        const SizeT len = GetLengthSSO_();
        JAM_ASSERT(IsSSO_(), "InsertToSSO_: must be in stack mode to insert to stack.");
        JAM_ASSERT(_pos <= len, "InsertToSSO_: _pos exceeds string length.");
        JAM_ASSERT(_str.GetLength() + len <= kSSOCap, "InsertToSSO_: combined length exceeds stack capacity.");

        ::memmove(m_str.sso + _pos + _str.GetLength(), m_str.sso + _pos, (len - _pos) * sizeof(TChar));
        ::memcpy(m_str.sso + _pos, _str.GetPtr(), _str.GetLength() * sizeof(TChar));
        ResizeSSO_(len + _str.GetLength());
    }

    void SSOToHeap_(
        const SizeT _newCap)
    {
        JAM_ASSERT(IsSSO_(), "SSOToHeap_: must be in stack mode to convert to heap.");
        JAM_ASSERT(_newCap > kSSOCap, "SSOToHeap_: _newCap must be greater than stack capacity.");

        TChar*      pHeap  = static_cast<TChar*>(Malloc((_newCap + 1) * sizeof(TChar)));
        const SizeT ssoLen = GetLengthSSO_();
        ::memcpy(pHeap, m_str.sso, ssoLen * sizeof(CharT));

        m_str.heap    = pHeap;
        m_str.heapCap = _newCap;
        m_str.bit     = kHeapEnabledBit;
        ResizeHeap_(ssoLen);
    }

    void EraseAtSSO_(
        const SizeT _pos,
        const SizeT _count)
    {
        JAM_ASSERT(IsSSO_(), "EraseAtSSO_: must be in stack mode to erase stack data.");
        const SizeT total = _pos + _count;
        const SizeT len   = GetLengthSSO_();
        JAM_ASSERT(_pos <= len, "EraseAtSSO_: _pos exceeds string length.");
        JAM_ASSERT(total <= len, "EraseAtSSO_: _pos + _count exceeds string length.");
        ::memmove(m_str.sso + _pos, m_str.sso + total, (len - total) * sizeof(TChar));
        ResizeSSO_(len - _count);
    }

    RawData m_str;   // do not zero-initialize for performance.
};

using String  = BasicString<char>;
using WString = BasicString<wchar_t>;

}   // namespace Jam

////////// std + external glue //////////

template<Jam::IsCharT CharT>
struct std::hash<Jam::BasicString<CharT>>
{
    std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return _str.Hash();
    }
};

template<Jam::IsCharT CharT>
struct ankerl::unordered_dense::hash<Jam::BasicString<CharT>>
{
    [[nodiscard]] std::size_t operator()(const Jam::String& _str) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(_str.GetPtr(), static_cast<size_t>(_str.GetLength()));
    }
};

template<>
struct fmt::formatter<Jam::String> : fmt::formatter<std::string_view>
{
    template<typename FormatContext>
    auto format(const Jam::String& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::string_view>::format({ _str.GetPtr(), static_cast<size_t>(_str.GetLength()) }, _ctx);
    }
};

template<>
struct fmt::formatter<Jam::WString, wchar_t> : fmt::formatter<std::wstring_view, wchar_t>
{
    template<typename FormatContext>
    auto format(const Jam::WString& _str, FormatContext& _ctx) const
    {
        return fmt::formatter<std::wstring_view, wchar_t>::format({ _str.GetPtr(), static_cast<size_t>(_str.GetLength()) }, _ctx);
    }
};