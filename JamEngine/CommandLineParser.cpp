#include "pch.h"
#include "CommandLineParser.h"

namespace Jam
{

CommandLineParser::CommandLineParser(
    const Int32 _argc,
    char**      _argv)
    : m_argv(_argv)
    , m_numArgs(_argc)
{
}

Int32 CommandLineParser::GetNumArguments() const
{
    return m_numArgs;
}

StringView CommandLineParser::GetArgument(
    const Int32 _index) const
{
    JAM_ASSERT(_index >= 0 && _index < m_numArgs, "Index out of bounds.");
    return { m_argv[_index] };
}

bool CommandLineParser::HasFlag(
    const StringView _flag) const
{
    JAM_ASSERT(_flag.StartsWith("--"), "Flag must start with '--'.");
    for (Int32 i = 0; i < m_numArgs; ++i)
    {
        if (StringView { m_argv[i] } == _flag)
        {
            return true;
        }
    }
    return false;
}

Optional<StringView> CommandLineParser::GetValue(
    const StringView _flag,
    const Int32      _index) const
{
    JAM_ASSERT(_flag.StartsWith("--"), "Flag must start with '--'.");
    for (Int32 i = 0; i < m_numArgs; ++i)
    {
        if (StringView { m_argv[i] } != _flag)
        {
            continue;
        }

        Int32 valueCount = 0;
        for (Int32 j = i + 1; j < m_numArgs; ++j)
        {
            if (StringView { m_argv[j] }.StartsWith("--"))
            {
                break;
            }

            if (valueCount == _index)
            {
                return StringView { m_argv[j] };
            }

            ++valueCount;
        }

        return kNullopt;
    }

    return kNullopt;
}

Optional<bool> CommandLineParser::GetBool(
    const StringView _flag,
    const Int32      _index) const
{
    const Optional<StringView> value = GetValue(_flag, _index);
    if (!value)
    {
        return kNullopt;
    }

    const StringView& str = *value;
    if (str == "true")
    {
        return true;
    }
    if (str == "false")
    {
        return false;
    }

    return kNullopt;
}

Optional<Int32> CommandLineParser::GetInt(
    const StringView _flag,
    const Int32      _index) const
{
    const Optional<StringView> value = GetValue(_flag, _index);
    if (!value)
    {
        return kNullopt;
    }

    return value->ParseAs<Int32>();
}

Optional<float> CommandLineParser::GetFloat(
    const StringView _flag,
    const Int32      _index) const
{
    const Optional<StringView> value = GetValue(_flag, _index);
    if (!value)
    {
        return kNullopt;
    }

    return value->ParseAs<float>();
}

Optional<StringView> CommandLineParser::GetString(
    const StringView _flag,
    const Int32      _index) const
{
    return GetValue(_flag, _index);
}

bool CommandLineParser::GetBoolOr(
    const StringView _flag,
    const bool       _value,
    const Int32      _index) const
{
    const Optional<bool> value = GetBool(_flag, _index);
    return value ? *value : _value;
}

Int32 CommandLineParser::GetIntOr(
    const StringView _flag,
    const Int32      _value,
    const Int32      _index) const
{
    const Optional<Int32> value = GetInt(_flag, _index);
    return value ? *value : _value;
}

float CommandLineParser::GetFloatOr(
    const StringView _flag,
    const float      _value,
    const Int32      _index) const
{
    const Optional<float> value = GetFloat(_flag, _index);
    return value ? *value : _value;
}

StringView CommandLineParser::GetStringOr(
    const StringView _flag,
    const StringView _value,
    const Int32      _index) const
{
    const Optional<StringView> value = GetString(_flag, _index);
    return value ? *value : _value;
}

}   // namespace Jam