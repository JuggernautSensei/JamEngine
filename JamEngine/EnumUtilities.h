// ReSharper disable CppTemplateParameterNeverUsed
#pragma once
#include "CTCStringHashMap.h"
#include "NameOf.h"

namespace Jam
{

template<typename TEnum>
    requires std::is_enum_v<TEnum>
constexpr UnderlyingT<TEnum> AsUnderlying(TEnum _value)
{
    return static_cast<UnderlyingT<TEnum>>(_value);
}

namespace Details
{

    constexpr SizeT kReflectionMin   = 0;
    constexpr SizeT kReflectionMax   = 255;
    constexpr SizeT kReflectionRange = kReflectionMax - kReflectionMin + 1;
    constexpr SizeT kInvalidSeq      = std::numeric_limits<SizeT>::max();

    static_assert(kReflectionMin < kReflectionMax, "Invalid enum reflection range.");
    static_assert(kReflectionMin >= Limits<SizeT>::kMin, "kReflectionMin is too small.");
    static_assert(kReflectionMax <= Limits<SizeT>::kMax, "kReflectionMax is too large.");

    template<typename T, T kEnum>
        requires std::is_enum_v<T>
    [[nodiscard]] constexpr ExtractResult ExtractName_(const bool _fullName)
    {
        constexpr StringView raw { JAM_PRETTY_FUNCTION };
        constexpr CharResult suffix = raw.RFind(">(");
        static_assert(suffix.IsSucceeded(), "Failed to find suffix in pretty function.");
        constexpr SizeT end = suffix.pos;

        const CharResult prefix = _fullName ? raw.RFind(",") : raw.RFind("::");
        JAM_ASSERT(prefix.IsSucceeded(), "Failed to find prefix in pretty function.");
        const SizeT start = prefix.pos + (_fullName ? 1 : 2);

        return { raw.GetPtr() + start, end - start };
    }

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    struct EnumNameHolder
    {
        static constexpr auto       kFullNameBuf = BuildNameBuffer<ExtractName_<TEnum, kEnum>(true)>();
        static constexpr auto       kNameBuf     = BuildNameBuffer<ExtractName_<TEnum, kEnum>(false)>();
        static constexpr StringView kFullName    = { kFullNameBuf.data(), kFullNameBuf.size() - 1 };
        static constexpr StringView kName        = { kNameBuf.data(), kNameBuf.size() - 1 };
    };

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr bool IsValidEnumValue()
    {
        constexpr StringView raw { JAM_PRETTY_FUNCTION };
        return !raw.Find(")0x").IsSucceeded();
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr SizeT CountEnumValues()
    {
        return []<size_t... I>(std::index_sequence<I...>) constexpr {
            return (0 + ... + (IsValidEnumValue<TEnum, static_cast<TEnum>(I)>() ? 1 : 0));
        }(std::make_index_sequence<kReflectionRange>());
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    struct EnumReflection
    {
        using Underlying                = UnderlyingT<TEnum>;
        static constexpr SizeT kCount   = CountEnumValues<TEnum>();
        static constexpr SizeT kLutSize = static_cast<SizeT>(2 * kCount * kInvCTCStringHashMapLoadFactor);

        Array<StringView, kCount>         fullNames  = {};
        Array<StringView, kCount>         names      = {};
        Array<TEnum, kCount>              values     = {};
        Array<SizeT, kReflectionRange>    rangeToSeq = {};
        CTCStringHashMap<SizeT, kLutSize> nameToSeq  = {};
        TEnum                             minValue   = static_cast<TEnum>(kReflectionMax);
        TEnum                             maxValue   = static_cast<TEnum>(kReflectionMin);
    };

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    constexpr void BuildEntryImpl_(
        EnumReflection<TEnum>& _refl,
        const SizeT            _rangeIdx,
        SizeT&                 _seq)
    {
        if constexpr (IsValidEnumValue<TEnum, kEnum>())
        {
            using H                     = EnumNameHolder<TEnum, kEnum>;
            _refl.fullNames[_seq]       = H::kFullName;
            _refl.names[_seq]           = H::kName;
            _refl.values[_seq]          = kEnum;
            _refl.rangeToSeq[_rangeIdx] = _seq;
            _refl.nameToSeq.Insert(H::kName, _seq);
            _refl.nameToSeq.Insert(H::kFullName, _seq);
            _refl.minValue = Min(_refl.minValue, kEnum);
            _refl.maxValue = Max(_refl.maxValue, kEnum);
            ++_seq;
        }
        else
        {
            _refl.rangeToSeq[_rangeIdx] = kInvalidSeq;
        }
    }

    template<typename TEnum, size_t... I>
    constexpr EnumReflection<TEnum> BuildReflection_(std::index_sequence<I...>)
    {
        EnumReflection<TEnum> refl = {};
        SizeT                 seq  = 0;
        (BuildEntryImpl_<TEnum, static_cast<TEnum>(I + kReflectionMin)>(refl, static_cast<SizeT>(I), seq), ...);
        return refl;
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr EnumReflection<TEnum> kReflection = BuildReflection_<TEnum>(std::make_index_sequence<kReflectionRange>());

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr SizeT FindSeq(const TEnum _value)
    {
        const SizeT rangeIdx = AsUnderlying(_value) - kReflectionMin;
        return (rangeIdx < kReflectionRange) ? kReflection<TEnum>.rangeToSeq[rangeIdx] : kInvalidSeq;
    }

}   // namespace Details

template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr bool IsValid(const TEnum _value)
{
    return Details::FindSeq(_value) != Details::kInvalidSeq;
}

// enum 값의 순서 인덱스를 반환
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT ToSequence(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return seq;
}

// enum 타입의 최솟값을 반환
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MinOf()
{
    return Details::kReflection<TEnum>.minValue;
}

// enum 타입의 최댓값을 반환
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MaxOf()
{
    return Details::kReflection<TEnum>.maxValue;
}

// enum 타입에 정의된 값의 개수를 반환
// ex) enum class eColor { Red = 1, Green = 2, Blue = 5 }; CountOf<eColor>() == 3
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT CountOf()
{
    return Details::kReflection<TEnum>.kCount;
}

// enum 값의 짧은 이름을 반환
// ex) eColor::Red -> "Red"
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView NameOf(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return Details::kReflection<TEnum>.names[seq];
}

// enum 값의 풀네임을 반환한다.
// ex) eColor::Red -> "eColor::Red"
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView FullNameOf(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return Details::kReflection<TEnum>.fullNames[seq];
}

// 모든 enum 값의 배열을 반환한다. 범위 기반 for에 사용 가능.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr const auto& RangesOf()
{
    return Details::kReflection<TEnum>.values;
}

// 문자열로 enum 값을 파싱한다. 짧은 이름/풀네임 모두 허용.
// ex) ParseAs<eColor>("Red") == eColor::Red
// ex) ParseAs<eColor>("Invalid") == kNullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> ParseAs(const StringView _str)
{
    const auto& lut = Details::kReflection<TEnum>.nameToSeq;
    if (!lut.Has(_str))
    {
        return kNullopt;
    }
    return Details::kReflection<TEnum>.values[lut.Get(_str)];
}

// 기저 타입 값으로 enum 값을 캐스팅한다. 정의되지 않은 값이면 kNullopt를 반환한다.
// ex) CastAs<eColor>(5) == eColor::Red
// ex) CastAs<eColor>(0) == kNullopt  (0이 정의되지 않았을 때)
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> CastAs(const UnderlyingT<TEnum> _value)
{
    const SizeT rangeIdx = _value - Details::kReflectionMin;
    if (rangeIdx >= Details::kReflectionRange)
    {
        return kNullopt;
    }

    const SizeT seq = Details::kReflection<TEnum>.rangeToSeq[rangeIdx];
    if (seq == Details::kInvalidSeq)
    {
        return kNullopt;
    }

    return Details::kReflection<TEnum>.values[seq];
}

}   // namespace Jam