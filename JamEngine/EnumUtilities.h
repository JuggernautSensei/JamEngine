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
    constexpr SizeT k_enumReflectionRangeMin   = 0;   // < if you need negative enum values, change this m_value accordingly
    constexpr SizeT k_enumReflectionRangeMax   = 127;
    constexpr SizeT k_enumReflectionRange      = k_enumReflectionRangeMax - k_enumReflectionRangeMin + 1;
    constexpr SizeT k_enumReflectionInvalidSeq = -1;

    static_assert(k_enumReflectionRangeMin < k_enumReflectionRangeMax, "Invalid enum reflection range.");
    static_assert(k_enumReflectionRangeMin >= Limits<SizeT>::k_min, "k_enumReflectionRangeMin is too small.");
    static_assert(k_enumReflectionRangeMax <= Limits<SizeT>::k_max, "k_enumReflectionRangeMax is too large.");

    // ReSharper disable once CppTemplateParameterNeverUsed
    template<typename T, T k_enum>
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

    template<typename T, T k_enum>
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

    template<typename TEnum, TEnum k_enum>
        requires std::is_enum_v<TEnum>
    struct EnumNameHolder
    {
        static constexpr auto       k_fullNameBuf = BuildNameBuffer<ExtractEnumFullName<TEnum, k_enum>()>();
        static constexpr auto       k_nameBuf     = BuildNameBuffer<ExtractEnumName<TEnum, k_enum>()>();
        static constexpr StringView k_fullName    = { k_fullNameBuf.data(), k_fullNameBuf.size() - 1 };
        static constexpr StringView k_name        = { k_nameBuf.data(), k_nameBuf.size() - 1 };
    };

    template<typename TEnum, TEnum k_enum>
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
        return CountEnumValuesImpl<TEnum>(std::make_index_sequence<k_enumReflectionRange>());
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    struct EnumReflection
    {
        using Underlying                   = UnderlyingT<TEnum>;
        static constexpr SizeT k_numValues = CountEnumValues<TEnum>();
        static constexpr SizeT k_lutSize   = NumericCast<SizeT>(2 * k_numValues * k_invConstexprStringHashMapLoadFactor);

        Array<StringView, k_numValues>           fullNames        = {};   // look up by seq
        Array<StringView, k_numValues>           names            = {};   // look up by seq
        Array<TEnum, k_numValues>                values           = {};   // look up by seq
        Array<SizeT, k_enumReflectionRange>      rangeIdxToSeqLut = {};   // look up by range index
        ConstexprStringHashMap<k_lutSize, SizeT> nameToSeqLut     = {};   // look up by title
        TEnum                                    minValue         = static_cast<TEnum>(k_enumReflectionRangeMax);
        TEnum                                    maxValue         = static_cast<TEnum>(k_enumReflectionRangeMin);
    };

    template<typename TEnum, TEnum k_enum>
        requires std::is_enum_v<TEnum>
    constexpr void BuildEnumReflectionImpl(
        EnumReflection<TEnum>& _refl,
        const SizeT            _rangeIdx,   // index in the reflection range not always valid
        SizeT&                 _seq         // sequence index = valid enum index
    )
    {
        if constexpr (IsValidEnumValue<TEnum, k_enum>())
        {
            using NameHolderT                 = EnumNameHolder<TEnum, k_enum>;
            _refl.fullNames[_seq]             = NameHolderT::k_fullName;
            _refl.names[_seq]                 = NameHolderT::k_name;
            _refl.values[_seq]                = k_enum;
            _refl.rangeIdxToSeqLut[_rangeIdx] = _seq;
            _refl.nameToSeqLut.Insert(NameHolderT::k_name, _seq);
            _refl.nameToSeqLut.Insert(NameHolderT::k_fullName, _seq);
            _refl.minValue = Min(_refl.minValue, k_enum);
            _refl.maxValue = Max(_refl.maxValue, k_enum);
            ++_seq;
        }
        else
        {
            _refl.rangeIdxToSeqLut[_rangeIdx] = k_enumReflectionInvalidSeq;
        }
    }

    template<typename TEnum, size_t... I>
    constexpr EnumReflection<TEnum> BuildEnumReflection(
        std::index_sequence<I...>)
    {
        EnumReflection<TEnum> refl = {};
        SizeT                 seq  = 0;
        (BuildEnumReflectionImpl<TEnum, static_cast<TEnum>(I + k_enumReflectionRangeMin)>(refl, NumericCast<SizeT>(I), seq), ...);
        return refl;
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr EnumReflection<TEnum> k_enumReflection = BuildEnumReflection<TEnum>(std::make_index_sequence<k_enumReflectionRange>());

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr SizeT FindLookUpSequenceOrInvalid(   // or k_enumReflectionInvalidIdx
        TEnum _value)
    {
        const SizeT rangeIdx = UnderlyingFrom(_value) - k_enumReflectionRangeMin;
        return (rangeIdx < k_enumReflectionRange) ? k_enumReflection<TEnum>.rangeIdxToSeqLut[rangeIdx] : k_enumReflectionInvalidSeq;
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
    return seq != Details::k_enumReflectionInvalidSeq;
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
// GetSequence(eColor::Red) -> 0
// GetSequence(eColor::Green) -> 1
// GetSequence(eColor::Blue) -> 2
// GetSequence(eColor::Yellow) -> 3
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT GetSequence(
    TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::k_enumReflectionInvalidSeq, "Invalid enum m_value.");
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
    return Details::k_enumReflection<TEnum>.minValue;
}

template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MaxOf()
{
    return Details::k_enumReflection<TEnum>.maxValue;
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
    return Details::k_enumReflection<TEnum>.k_numValues;
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
    TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::k_enumReflectionInvalidSeq, "Invalid enum m_value.");
    return Details::k_enumReflection<TEnum>.names[seq];
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
    TEnum _value)
{
    const SizeT seq = Details::FindLookUpSequenceOrInvalid<TEnum>(_value);
    JAM_ASSERT(seq != Details::k_enumReflectionInvalidSeq, "Invalid enum m_value.");
    return Details::k_enumReflection<TEnum>.fullNames[seq];
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
// EnumRanges<eColor>() -> { eColor::Red, eColor::Green, eColor::Blue, eColor::Yellow }
// for (eColor color : EnumRanges<eColor>()) { ... }
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr const auto& EnumRanges()
{
    return Details::k_enumReflection<TEnum>.values;
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
// ParseAsIntegral<eColor>("Red") -> eColor::Red
// ParseAsIntegral<eColor>("eColor::Red") -> eColor::Red
// ParseAsIntegral<eColor>("Invalid") -> k_nullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> ParseAs(
    const StringView _str)
{
    const auto& lut = Details::k_enumReflection<TEnum>.nameToSeqLut;
    if (lut.Has(_str))
    {
        const SizeT seq = lut.Get(_str);
        return Details::k_enumReflection<TEnum>.values[seq];
    }
    return k_nullopt;
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
// EnumCast<eColor>(5) -> eColor::Red
// EnumCast<eColor>(20) -> eColor::Yellow
// EnumCast<eColor>(0) -> k_nullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> EnumCast(
    const UnderlyingT<TEnum> _value)
{
    const SizeT rangeIdx = _value - Details::k_enumReflectionRangeMin;
    if (rangeIdx >= Details::k_enumReflectionRange)
    {
        return k_nullopt;
    }

    const SizeT seq = Details::k_enumReflection<TEnum>.rangeIdxToSeqLut[rangeIdx];
    if (seq == Details::k_enumReflectionInvalidSeq)
    {
        return k_nullopt;
    }

    return Details::k_enumReflection<TEnum>.values[seq];
}

}   // namespace Jam