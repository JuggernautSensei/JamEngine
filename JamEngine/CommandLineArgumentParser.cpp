#include "pch.h"
#include "CommandLineArgumentParser.h"

namespace Jam
{

CommandLineArgumentParser::CommandLineArgumentParser(
    const Int32 _argc,
    char**      _argv)
    : m_numArgs(_argc)
    , m_args(_argv)
{
}

Int32 CommandLineArgumentParser::GetNumArguments() const
{
    return m_numArgs;
}

StringView CommandLineArgumentParser::GetArgument(
    const Int32 _index) const
{
    JAM_ASSERT(_index >= 0 && _index < m_numArgs, "Index out of bounds. Index: {}, NumArgs: {}", _index, m_numArgs);
    return { m_args[_index] };
}

}   // namespace Jam