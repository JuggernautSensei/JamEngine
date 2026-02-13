#include "pch.h"

enum class eColor
{
    Red,
    Green,
    Blue
};
//
// template<typename TEnum, TEnum k_enum>
// class EnumNameExtractor
//{
// public:
//    static constexpr StringView GetName()
//    {
//        return { k_name.data(), static_cast<Int32>(k_name.size()) - 1 };
//    }
//
// private:
//    static constexpr auto ExtractName()
//    {
//        constexpr StringView      raw { __FUNCSIG__ };
//        constexpr StringView      prefixEnum { "EnumNameExtractor<enum " };
//        constexpr StringView      prefixEnumClass { "EnumNameExtractor<enum class " };
//        constexpr Optional<Int32> beginEnumIdx      = raw.FindFirstOf(prefixEnumClass);
//        constexpr Optional<Int32> beginEnumClassIdx = raw.FindFirstOf(prefixEnum);
//        constexpr Int32           beginIdx          = beginEnumClassIdx ? beginEnumClassIdx.GetValue() + prefixEnumClass.GetLength() : beginEnumIdx.GetValue() + prefixEnum.GetLength();
//        constexpr Int32           endIdx            = raw.FindLastOf('>').GetValue();
//        constexpr Int32           len               = endIdx - beginIdx;
//        return false;
//    }
//
//    static constexpr auto k_name = ExtractName();
//};

int main()
{
    //::printf("Enum name: %s\n", EnumNameExtractor<eColor, eColor::Green>::GetName().GetPtr());
    constexpr StringView raw { __FUNCSIG__ };
    constexpr StringView prefixEnum { "EnumNameExtractor<enum " };
    constexpr StringView prefixEnumClass { "EnumNameExtractor<enum class " };
    auto                 beginEnumIdx = raw.FindFirstOf(prefixEnumClass);
    // constexpr Optional<Int32> beginEnumClassIdx = raw.FindFirstOf(prefixEnum);
    // constexpr Int32           beginIdx          = beginEnumClassIdx ? beginEnumClassIdx.GetValue() + prefixEnumClass.GetLength() : beginEnumIdx.GetValue() + prefixEnum.GetLength();
    // constexpr Int32           endIdx            = raw.FindLastOf('>').GetValue();
    // constexpr Int32           len               = endIdx - beginIdx;
    return 0;
}
