// ReSharper disable CppTemplateParameterNeverUsed
#pragma once

namespace Jam
{

namespace Details
{
    constexpr SizeT kMinTraits  = 0;
    constexpr SizeT kMaxTraits  = 255;
    constexpr SizeT kNumTraits  = kMaxTraits - kMinTraits + 1;
    constexpr SizeT kInvalidSeq = Limits<SizeT>::kMax;

    static_assert(kMinTraits < kMaxTraits, "Invalid enum reflection range.");
    static_assert(kMinTraits >= Limits<SizeT>::kMin, "kReflectionMin is too small.");
    static_assert(kMaxTraits <= Limits<SizeT>::kMax, "kReflectionMax is too large.");

    constexpr float kNameToSeqTableLoadFactor    = 0.7f;
    constexpr float kInvNameToSeqTableLoadFactor = 1.f / kNameToSeqTableLoadFactor;

    template<SizeT kSize>
    class NameToSeqTable
    {
        static_assert(kSize > 0, "NameToSeqTable size must be greater than 0.");

        struct Node
        {
            StringView key       = {};
            SizeT      seq       = {};
            bool       bOccupied = false;
        };

        static constexpr SizeT kInvalidIdx = Limits<SizeT>::kMax;

    public:
        constexpr NameToSeqTable() = default;

        constexpr void Insert(
            const StringView _key,
            const SizeT      _value)
        {
            const UInt64 hash  = _key.Hash();
            SizeT        index = hash % kSize;

            for (SizeT i = 0; i < kSize; ++i)
            {
                Node& node = m_nodes[index];
                if (!node.bOccupied)
                {
                    node.key       = _key;
                    node.seq       = _value;
                    node.bOccupied = true;
                    ++m_numNodes;
                    return;
                }
                if (node.key == _key)
                {
                    node.seq = _value;
                    return;
                }

                index = (index + 1) % kSize;
            }
        }

        constexpr SizeT GetOrInvalid(
            const StringView _key) const
        {
            const SizeT index = FindIndexOrInvalid_(_key);
            return index != kInvalidIdx ? m_nodes[index].seq : kInvalidSeq;
        }

    private:
        [[nodiscard]] constexpr SizeT FindIndexOrInvalid_(
            const StringView _key) const
        {
            if (m_numNodes == 0)
            {
                return kInvalidIdx;
            }

            const UInt64 hash  = _key.Hash();
            SizeT        index = hash % kSize;

            for (SizeT i = 0; i < kSize; ++i)
            {
                const Node& node = m_nodes[index];
                if (!node.bOccupied)
                {
                    return kInvalidIdx;
                }

                if (node.key == _key)
                {
                    return index;
                }

                index = (index + 1) % kSize;
            }

            return kInvalidIdx;
        }

        Array<Node, kSize> m_nodes    = {};
        SizeT              m_numNodes = 0;
    };

    template<typename T, T kEnum, bool kFullName>
        requires std::is_enum_v<T>
    [[nodiscard]] constexpr ExtractResult ExtractName_()
    {
        constexpr StringView      raw    = JAM_PRETTY_FUNCTION;
        constexpr Optional<SizeT> suffix = raw.RFind(">(");
        static_assert(suffix, "Failed to find suffix in pretty function.");
        constexpr SizeT end = suffix.GetValue();

        constexpr Optional<SizeT> prefix = kFullName ? raw.RFind(",") : raw.RFind("::");
        static_assert(prefix, "Failed to find prefix in pretty function.");
        constexpr SizeT start = prefix.GetValue() + (kFullName ? 1 : 2);

        return { raw.GetPtr() + start, end - start };
    }

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    struct EnumNameHolder
    {
        static constexpr auto       kFullNameBuf = BuildNameBuffer<ExtractName_<TEnum, kEnum, true>()>();
        static constexpr auto       kNameBuf     = BuildNameBuffer<ExtractName_<TEnum, kEnum, false>()>();
        static constexpr StringView kFullName    = { kFullNameBuf.data(), kFullNameBuf.size() - 1 };
        static constexpr StringView kName        = { kNameBuf.data(), kNameBuf.size() - 1 };
    };

    template<typename TEnum, TEnum kEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr bool IsValidEnumImpl()
    {
        constexpr StringView raw = JAM_PRETTY_FUNCTION;
        return !raw.Find(")0x").HasValue();
    }

    template<typename TEnum, SizeT I>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr bool IsValidEnum()
    {
        using UT = UnderlyingT<TEnum>;
        if constexpr (I < static_cast<SizeT>(Limits<UT>::kLowest) || I > static_cast<SizeT>(Limits<UT>::kHighest))
        {
            return false;
        }
        else
        {
            return IsValidEnumImpl<TEnum, static_cast<TEnum>(I)>();
        }
    }

    template<typename TEnum, SizeT I>
        requires std::is_enum_v<TEnum>
    constexpr SizeT CalcNumEnumsImpl()
    {
        return IsValidEnum<TEnum, I>() ? 1 : 0;
    }

    template<typename TEnum, SizeT... I>
        requires std::is_enum_v<TEnum>
    constexpr SizeT CalcNumEnums(std::index_sequence<I...>)
    {
        return (0 + ... + CalcNumEnumsImpl<TEnum, I>());
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    struct EnumTraits
    {
        using UT                         = UnderlyingT<TEnum>;
        static constexpr SizeT kNumEnums = CalcNumEnums<TEnum>(std::make_index_sequence<kNumTraits>());
        static constexpr SizeT kLutSize  = static_cast<SizeT>(2 * kNumEnums * kInvNameToSeqTableLoadFactor);

        Array<StringView, kNumEnums> fullNames       = {};                                         // seq 순서대로 저장된 풀네임 배열
        Array<StringView, kNumEnums> names           = {};                                         // seq 순서대로 저장된 짧은 이름 배열
        Array<TEnum, kNumEnums>      values          = {};                                         // seq 순서대로 저장된 enum 값 배열
        Array<SizeT, kNumTraits>     rangeToSeqTable = {};                                         // 범위 인덱스(underlying - kReflectionMin)를 seq로 변환하는 LUT
        NameToSeqTable<kLutSize>     nameToSeqTable  = {};                                         // 이름 문자열을 seq로 조회하는 해시 테이블 (짧은이름/풀네임 모두 등록)
        TEnum                        minValue        = static_cast<TEnum>(Limits<UT>::kHighest);   // UT 최댓값
        TEnum                        maxValue        = static_cast<TEnum>(Limits<UT>::kLowest);    // UT 최솟값
    };

    template<typename TEnum, SizeT I>
        requires std::is_enum_v<TEnum>
    constexpr void BuildTraitsImpl(
        EnumTraits<TEnum>& _traits,
        SizeT&             _seq)
    {
        constexpr SizeT range = I - kMinTraits;
        if constexpr (IsValidEnum<TEnum, I>())
        {
            constexpr TEnum e = static_cast<TEnum>(I);
            using HolderT     = EnumNameHolder<TEnum, e>;

            _traits.fullNames[_seq]        = HolderT::kFullName;
            _traits.names[_seq]            = HolderT::kName;
            _traits.values[_seq]           = e;
            _traits.rangeToSeqTable[range] = _seq;
            _traits.nameToSeqTable.Insert(HolderT::kName, _seq);
            _traits.nameToSeqTable.Insert(HolderT::kFullName, _seq);
            _traits.minValue = Min(_traits.minValue, e);
            _traits.maxValue = Max(_traits.maxValue, e);
            ++_seq;
        }
        else
        {
            _traits.rangeToSeqTable[range] = kInvalidSeq;
        }
    }

    template<typename TEnum, SizeT... I>
    constexpr EnumTraits<TEnum> BulidTraits(std::index_sequence<I...>)
    {
        EnumTraits<TEnum> traits = {};
        SizeT             seq    = 0;
        (BuildTraitsImpl<TEnum, I + kMinTraits>(traits, seq), ...);
        return traits;
    }

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    constexpr EnumTraits<TEnum> kTraits = BulidTraits<TEnum>(std::make_index_sequence<kNumTraits>());

    template<typename TEnum>
        requires std::is_enum_v<TEnum>
    [[nodiscard]] constexpr SizeT FindSeq(const TEnum _value)
    {
        const SizeT range = static_cast<UnderlyingT<TEnum>>(_value) - kMinTraits;
        if (range < kMinTraits || range > kMaxTraits)
        {
            return kInvalidSeq;
        }

        return kTraits<TEnum>.rangeToSeqTable[range];
    }

}   // namespace Details

// enum 값의 underlying 타입 값을 반환한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
constexpr UnderlyingT<TEnum> AsUnderlying(TEnum _value)
{
    return static_cast<UnderlyingT<TEnum>>(_value);
}

// _value가 enum에 정의된 유효한 값인지 확인한다.
// underlying 값이 [kReflectionMin, kReflectionMax] 밖이면 항상 false다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr bool IsValid(const TEnum _value)
{
    return Details::FindSeq(_value) != Details::kInvalidSeq;
}

// enum 값의 선언 순서 기반 인덱스(0-origin)를 반환한다.
// ex) enum class eColor { Red=1, Green=2, Blue=5 }; AsSequence(eColor::Green) == 1
// 유효하지 않은 값이면 단언 실패한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT AsSequence(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return seq;
}

// enum 타입에 정의된 값 중 underlying 기준 최솟값을 반환한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MinOf()
{
    return Details::kTraits<TEnum>.minValue;
}

// enum 타입에 정의된 값 중 underlying 기준 최댓값을 반환한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr TEnum MaxOf()
{
    return Details::kTraits<TEnum>.maxValue;
}

// enum 타입에 정의된 값의 개수를 반환한다.
// ex) enum class eColor { Red=1, Green=2, Blue=5 }; CountOf<eColor>() == 3
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr SizeT CountOf()
{
    return Details::kTraits<TEnum>.kNumEnums;
}

// enum 값의 짧은 이름을 반환한다.
// ex) eColor::Red → "Red"
// 유효하지 않은 값이면 단언 실패한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView NameOf(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return Details::kTraits<TEnum>.names[seq];
}

// enum 값의 스코프 포함 풀네임을 반환한다.
// ex) eColor::Red → "eColor::Red"
// 유효하지 않은 값이면 단언 실패한다.
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr StringView FullNameOf(const TEnum _value)
{
    const SizeT seq = Details::FindSeq(_value);
    JAM_ASSERT(seq != Details::kInvalidSeq, "Invalid enum value.");
    return Details::kTraits<TEnum>.fullNames[seq];
}

// 정의된 모든 enum 값의 배열을 선언 순서대로 반환한다.
// 범위 기반 for에 사용 가능하다.
// ex) for (auto v : RangesOf<eColor>()) { ... }
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr const auto& RangesOf()
{
    return Details::kTraits<TEnum>.values;
}

// 문자열로 enum 값을 파싱한다. 짧은 이름과 풀네임을 모두 허용한다.
// ex) ParseAs<eColor>("Red")        == eColor::Red
// ex) ParseAs<eColor>("eColor::Red") == eColor::Red
// ex) ParseAs<eColor>("Invalid")    == kNullopt
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> ParseAs(const StringView _str)
{
    const auto& lut = Details::kTraits<TEnum>.nameToSeqTable;
    const SizeT seq = lut.GetOrInvalid(_str);
    if (seq == Details::kInvalidSeq)
    {
        return kNullopt;
    }

    return Details::kTraits<TEnum>.values[seq];
}

// 기저 타입 값을 enum으로 안전하게 캐스팅한다.
// 정의되지 않은 값이거나 [kReflectionMin, kReflectionMax] 범위 밖이면 kNullopt를 반환한다.
//
// 주의: 음수 underlying type은 지원하지 않는다.
//       kReflectionMin = 0이므로 음수 _value는 SizeT 변환 시 언더플로우가 발생한다.
//       따라서 음수 enum 값을 가진 타입에는 사용하지 않아야 한다.
//
// ex) CastAs<eColor>(2) == eColor::Green
// ex) CastAs<eColor>(0) == kNullopt  (0이 정의되지 않았을 때)
template<typename TEnum>
    requires std::is_enum_v<TEnum>
[[nodiscard]] constexpr Optional<TEnum> CastAs(const UnderlyingT<TEnum> _value)
{
    const SizeT v = static_cast<SizeT>(_value);
    if (v < Details::kMinTraits || v > Details::kMaxTraits)
    {
        return kNullopt;
    }

    const SizeT range = v - Details::kMinTraits;
    const SizeT seq   = Details::kTraits<TEnum>.rangeToSeqTable[range];
    if (seq == Details::kInvalidSeq)
    {
        return kNullopt;
    }

    return Details::kTraits<TEnum>.values[seq];
}

}   // namespace Jam