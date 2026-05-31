#pragma once

namespace Jam
{

// very simple & light weight command line argument parser :)

// parsing rules:
// App.exe --flag1 --flag2 value --flag3 value1 value2 value3 ...
// case 1: --flag1 is just a flag.
// case 2: --flag2 has a single value, which is "value".
// case 3: --flag3 has multiple values, which are "value1", "value2", "value3", ...
//
// value type:
// bool: --flag true, --flag false
// string: --flag "string", --flag string
// int: --flag 123       ("123" is string arg)
// float: --flag 1.23    ("1.23" is string arg)
class CommandLineParser
{
public:
    CommandLineParser(Int32 _argc, char** _argv);

    [[nodiscard]] StringView GetArgument(Int32 _index) const;
    [[nodiscard]] Int32      GetNumArguments() const;

    [[nodiscard]] bool                 HasFlag(StringView _flag) const;
    [[nodiscard]] Optional<StringView> GetValue(StringView _flag, Int32 _index = 0) const;

    [[nodiscard]] Optional<bool>       GetBool(StringView _flag, Int32 _index = 0) const;
    [[nodiscard]] Optional<Int32>      GetInt(StringView _flag, Int32 _index = 0) const;
    [[nodiscard]] Optional<float>      GetFloat(StringView _flag, Int32 _index = 0) const;
    [[nodiscard]] Optional<StringView> GetString(StringView _flag, Int32 _index = 0) const;

    [[nodiscard]] bool       GetBoolOr(StringView _flag, bool _value, Int32 _index = 0) const;
    [[nodiscard]] Int32      GetIntOr(StringView _flag, Int32 _value, Int32 _index = 0) const;
    [[nodiscard]] float      GetFloatOr(StringView _flag, float _value, Int32 _index = 0) const;
    [[nodiscard]] StringView GetStringOr(StringView _flag, StringView _value, Int32 _index = 0) const;

private:
    char** m_argv    = nullptr;
    Int32  m_numArgs = 0;
};

}   // namespace Jam