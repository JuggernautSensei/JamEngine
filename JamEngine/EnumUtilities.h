// ReSharper disable CppTemplateParameterNeverUsed
#pragma once
#include "ConstexprStringHashMap.h"
#include "NameOf.h"

namespace Jam
{

template<typename TEnum>
    requires std::is_enum_v<TEnum>
constexpr UnderlyingT<TEnum> UnderlyingFrom(TEnum _value)
{
    return static_cast<UnderlyingT<TEnum>>(_value);
}

namespace Details
{

    // config
    constexpr SizeT kEnumReflectionRangeMin   = 0;   // < if you need negative enum values, change this m_value accordingly
    constexpr SizeT kEnumReflectionRangeMax   = 255;
    constexpr SizeT kEnumReflectionRange      = kEnumReflectionRangeMax - kEnumReflectionRangeMin + 1;
    constexpr SizeT kEnumReflectionInvalidSeq = -1;

    static_assert(kEnumReflectionRangeMin < kEnumReflectionRangeMax, "Invalid enum reflection range.");
    static_assert(kEnumReflectionRangeMin >= Limits<SizeT>::kMin, "kEnumReflectionRangeMin is too small.");
    static_assert(kEnumReflectionRangeMax <= Limits<SizeT>::kMax, "kEnumReflectionRangeMax is too large.");

    // ReSharper disable once CppTemplateParameterNeverUsed
    template<typename T, T kEnum>
        requires std::is_enum_v<T>
    [[nodiscard]] constexpr ExtractResult ExtractEnumName()
    {
        constexpr StringView raw { JAM_PRETTY_FUNCTION };
        constexpr CharResult suffixResult = raw.RFind(">(");
        static_assert(suffixResult.IsSucceeded(), "Failed to extract enum title from pretty function.");
        constexpr SizeT      end          = suffixResult.pos;
        constexpr CharResult prefixResult = raw.RFind("::");
        static_assert(prefixResult.IsSucceeded(), "Failed to extract enum title from pretty function.");
        constexpr SizeT start = prefixResult.pos + 2;
        return { raw.GetPtr() + start, end - start };
    }

    template<typename T, T kEnum>
    [[nodiscard]] constexpr ExtractResult ExtractEnumFullName()
    {
        constexpr StringView raw { JAM_PRETTY_FUNCTION };
        constexpr CharResult suffixResult = raw.RFind(">(");
        static_assert(suffixResult.IsSucceeded(), "Failed to extract enum title from pretty function.");
        constexpr SizeT      end          = suffixResult.pos;
        constexpr CharResult prefixResult = raw.RFind(",");
        static_assert(prefixResult.IsSucceeded(), "Failed to extract enum title from pretty function.");
        constexpr SizeT start = prefixResult.pos + 1;
        return { raw.GetPtr() + start, end - start };
    }

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    struct EnumNameHolder
    {
        static constexpr auto       kFullNameBuf = BuildNameBuffer<ExtractEnumFullName<TEnum, kEnum>()>();
        static constexpr auto       kNameBuf     = BuildNameBuffer<ExtractEnumName<TEnum, kEnum>()>();
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

    template<typename TEnum, size_t... I>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr SizeT CountEnumValuesImpl(std::index_sequence<I...>)
    {
        return (0 + ... + (IsValidEnumValue<TEnum, static_cast<TEnum>(I)>() ? 1 : 0));
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr SizeT CountEnumValues()
    {
        return CountEnumValuesImpl<TEnum>(std::make_index_sequence<kEnumReflectionRange>());
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    struct EnumReflection
    {
        using Underlying                   = UnderlyingT<TEnum>;
        static constexpr SizeT kNumValues = CountEnumValues<TEnum>();
        static constexpr SizeT kLutSize   = static_cast<SizeT>(2 * kNumValues * kInvConstexprStringHashMapLoadFactor);

        Array<StringView, kNumValues>           fullNames        = {};   // look up by seq
        Array<StringView, kNumValues>           names            = {};   // look up by seq
        Array<TEnum, kNumValues>                values           = {};   // look up by seq
        Array<SizeT, kEnumReflectionRange>      rangeIdxToSeqLut = {};   // look up by range index
        ConstexprStringHashMap<SizeT, kLutSize> nameToSeqLut     = {};   // look up by title
        TEnum                                    minValue         = static_cast<TEnum>(kEnumReflectionRangeMax);
        TEnum                                    maxValue         = static_cast<TEnum>(kEnumReflectionRangeMin);
    };

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    constexpr void BuildEnumReflectionImpl(
        EnumReflection<TEnum>& _refl,
        const SizeT            _rangeIdx,   // index in the reflection range not always valid
        SizeT&                 _seq         // sequence index = valid enum index
    )
    {
        if constexpr (IsValidEnumValue<TEnum, kEnum>())
        {
            using NameHolderT                 = EnumNameHolder<TEnum, kEnum>;
            _refl.fullNames[_seq]             = NameHolderT::kFullName;
            _refl.names[_seq]                 = NameHolderT::kName;
            _refl.values[_seq]                = kEnum;
            _refl.rangeIdxToSeqLut[_rangeIdx] = _seq;
            _refl.nameToSeqLut.Insert(NameHolderT::kName, _seq);
            _refl.nameToSeqLut.Insert(NameHolderT::kFullName, _seq);
            _refl.minValue = Min(_refl.minValue, kEnum);
            _refl.maxValue = Max(_refl.maxValue, kEnum);
            ++_seq;
        }
        else
        {
            _refl.rangeIdxToSeqLut[_rangeIdx] = kEnumReflectionInvalidSeq;
        }
    }

    template<typename TEnum, size_t... I>
    constexpr EnumReflection<TEnum> BuildEnumReflection(
        std::index_sequence<I...>)
    {
        EnumReflection<TEnum> refl = {};
        SizeT                 seq  = 0;
        (BuildEnumReflectionImpl<TEnum, static_cast<TEnum>(I + kEnumReflectionRangeMin)>(refl, static_cast<SizeT>(I), seq), ...);
        return refl;
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr EnumReflection<TEnum> kEnumReflection = BuildEnumReflection<TEnum>(std::make_index_sequence<kEnumReflectionRange>());

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr SizeT FindLookUpSequenceOrInvalid(   // or kEnumReflectionInvalidIdx
        const TEnum _value)
    {
        const SizeT rangeIdx = UnderlyingFrom(_value) - kEnumReflectionRangeMin;
        return (rangeIdx < kEnumReflectionRange) ? kEnumReflection<TEnum>.rangeIdxToSeqLut[rangeIdx] : kEnumReflectionInvalidSeq;
    }

}   // namespace Details

// check is valid enum m_value
// ex)
// enum class eColor
// {
//     Red = 0,
//     Green = 1,
//     Blue = 2,
//     Yellow = 3,
// };
// IsValid(eColor::Red) -> true
// IsValid(static_cast<eColor>(-1)) -> false
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT IsValid(
    TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    return seq != Details::kEnumReflectionInvalidSeq;
}

// get the sequence index of the enum m_value.
// ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// ToSequence(eColor::Red) -> 0
// ToSequence(eColor::Green) -> 1
// ToSequence(eColor::Blue) -> 2
// ToSequence(eColor::Yellow) -> 3
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT ToSequence(
    TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::kEnumReflectionInvalidSeq, "Invalid enum m_value.");
    return seq;
}

// get the minimum enum m_value defined in the enum type.
// // ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// MinOf<eColor>() -> eColor::Red
// MaxOf<eColor>() -> eColor::Yellow
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MinOf()
{
    return Details::kEnumReflection<TEnum>.minValue;
}

template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MaxOf()
{
    return Details::kEnumReflection<TEnum>.maxValue;
}

// get the number of enum values defined in the enum type.
// ex)
// enum class eColor
// {
//     Red,
//     Green,
//     Blue,
//     Yellow,
// };
// CountOf<eColor>() -> 4
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT CountOf()
{
    return Details::kEnumReflection<TEnum>.kNumValues;
}

// get the title of the enum m_value.
// ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// NameOf(eColor::Red) -> "Red"
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView NameOf(
    const TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::kEnumReflectionInvalidSeq, "Invalid enum m_value.");
    return Details::kEnumReflection<TEnum>.names[seq];
}

// get the full title of the enum m_value. full title includes the enum type title and the enum m_value title.
// ex)
// namespace Jam
// {
//     enum class eColor
//     {
//         Red = 5,
//         Green = 10,
//         Blue = 15,
//         Yellow = 20,
//     };
// }
// FullNameOf(eColor::Red) -> "Jam::eColor::Red"
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView FullNameOf(
    const TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::kEnumReflectionInvalidSeq, "Invalid enum m_value.");
    return Details::kEnumReflection<TEnum>.fullNames[seq];
}

// get the enum m_value array. this is useful for iterating over all enum values defined in the enum type.
// ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// RangesOf<eColor>() -> { eColor::Red, eColor::Green, eColor::Blue, eColor::Yellow }
// for (eColor color : RangesOf<eColor>()) { ... }
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr const auto& RangesOf()
{
    return Details::kEnumReflection<TEnum>.values;
}

// parse enum m_value from string. this is useful for parsing enum m_value from config file or user input.
// ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// ParseAs<eColor>("Red") -> eColor::Red
// ParseAs<eColor>("eColor::Red") -> eColor::Red
// ParseAs<eColor>("Invalid") -> kNullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> ParseAs(
    const StringView _str)
{
    const auto& lut = Details::kEnumReflection<TEnum>.nameToSeqLut;
    if (lut.Has(_str))
    {
        const SizeT seq = lut.Get(_str);
        return Details::kEnumReflection<TEnum>.values[seq];
    }
    return kNullopt;
}

// cast from underlying m_value to enum m_value. this is useful for parsing enum m_value from config file or user input when the enum m_value is represented as its underlying m_value.
// ex)
// enum class eColor
// {
//     Red = 5,
//     Green = 10,
//     Blue = 15,
//     Yellow = 20,
// };
// CastAs<eColor>(5) -> eColor::Red
// CastAs<eColor>(20) -> eColor::Yellow
// CastAs<eColor>(0) -> kNullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> CastAs(
    const UnderlyingT<TEnum> _value)
{
    const SizeT rangeIdx = _value - Details::kEnumReflectionRangeMin;
    if (rangeIdx >= Details::kEnumReflectionRange)
    {
        return kNullopt;
    }

    const SizeT seq = Details::kEnumReflection<TEnum>.rangeIdxToSeqLut[rangeIdx];
    if (seq == Details::kEnumReflectionInvalidSeq)
    {
        return kNullopt;
    }

    return Details::kEnumReflection<TEnum>.values[seq];
}

}   // namespace Jam