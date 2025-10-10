#pragma once

namespace jam
{

#ifdef JAM_TEXT_ENCODING_UNICODE
using tchar        = wchar_t;
using tstring      = std::wstring;
using tstring_view = std::wstring_view;
#    define JAM_TTEXT(x) L##x
#else   // JAM_TEXT_ENCODING_MULTIBYTE
using tchar        = char;
using tstring      = std::string;
using tstring_view = std::string_view;
#    define JAM_TTEXT(x) x
#endif

JAM_NODISCARD constexpr UInt32 HashOf(const std::string_view _str)
{
    return entt::hashed_string { _str.data(), _str.size() }.value();
}

JAM_NODISCARD constexpr UInt32 HashOf(const std::wstring_view _wStr)
{
    return entt::hashed_wstring { _wStr.data(), _wStr.size() }.value();
}

JAM_NODISCARD constexpr UInt32 operator""_hs(const char* _str, const size_t _size)
{
    return HashOf(std::string_view { _str, _size });
}

JAM_NODISCARD constexpr UInt32 operator""_hs(const wchar_t* _str, const size_t _size)
{
    return HashOf(std::wstring_view { _str, _size });
}

JAM_NODISCARD std::wstring ToLower(std::wstring_view _wStr);
JAM_NODISCARD std::string ToLower(std::string_view _str);
JAM_NODISCARD Result<std::string> ToUTF8(std::wstring_view _wstr);
JAM_NODISCARD Result<std::wstring> ToUTF16(std::string_view _str);
JAM_NODISCARD std::vector<std::string> Split(std::string_view _str, std::string_view _delimiter);

template<typename T>
    requires std::is_integral_v<T>
Result<T> StringToInt(const std::string_view _str)
{
    if (_str.empty())
    {
        SetErrorString("StringToInt: input string is empty");   
        return eErrorCode::InvalidArgument;
    }

    bool   bNegative = false;
    size_t start     = 0;

    if constexpr (std::is_unsigned_v<T>)
    {
        if (_str[0] == '-')
        {
            SetErrorString("StringToInt: negative sign is not allowed for unsigned type");
            return eErrorCode::InvalidArgument;   // unsigned에 음수 부호 금지
        }
    }
    else
    {
        if (_str[start] == '-')
        {
            bNegative = true;
            ++start;
        }
        else if (_str[start] == '+')
        {
            ++start;
        }
    }

    if (start >= _str.size())
    {
        SetErrorString("StringToInt: no digits found in the input string");
        return eErrorCode::InvalidArgument;   // 부호만 있고 숫자 없음
    }

    T output = 0;
    for (size_t i = start; i < _str.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(_str[i]);
        if (!std::isdigit(c))
        {
            SetErrorString("StringToInt: invalid character '{}' found in the input string", _str[i]);
            return eErrorCode::InvalidArgument;
        }

        T digit = static_cast<T>(c - '0');
        if constexpr (std::is_signed_v<T>)
        {
            // 부호 있는 타입
            if (!bNegative)
            {
                if (output > (std::numeric_limits<T>::max() - digit) / 10)
                {
                    SetErrorString("StringToInt: overflow occurred while converting string to integer");
                    return eErrorCode::Overflow;
                }
                output = output * 10 + digit;
            }
            else
            {
                if (output < (std::numeric_limits<T>::min() + digit) / 10)
                {
                    SetErrorString("StringToInt: underflow occurred while converting string to integer");
                    return eErrorCode::Overflow;
                }
                output = output * 10 - digit;
            }
        }
        else
        {
            // 부호 없는 타입
            if (output > (std::numeric_limits<T>::max() - digit) / 10)
            {
                SetErrorString("StringToInt: overflow occurred while converting string to integer");
                return eErrorCode::Overflow;
            }
            output = output * 10 + digit;
        }
    }

    return output;
}


}   // namespace jam