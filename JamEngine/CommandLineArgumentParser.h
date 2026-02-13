#pragma once

namespace Jam
{

// very simple command line argument parser :)
class CommandLineArgumentParser
{
public:
    CommandLineArgumentParser(Int32 _argc, char** _argv);

    [[nodiscard]] Int32      GetNumArguments() const;
    [[nodiscard]] StringView GetArgument(Int32 _index) const;

private:
    Int32  m_numArgs = 0;
    char** m_args    = nullptr;
};

}   // namespace Jam
