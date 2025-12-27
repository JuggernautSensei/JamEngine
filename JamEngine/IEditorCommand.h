#pragma once

namespace jam
{

class IEditorCommand
{
public:
    IEditorCommand()          = default;
    virtual ~IEditorCommand() = default;

    IEditorCommand(const IEditorCommand&)            = default;
    IEditorCommand(IEditorCommand&&)                 = default;
    IEditorCommand& operator=(const IEditorCommand&) = default;
    IEditorCommand& operator=(IEditorCommand&&)      = default;

    virtual void Execute() = 0;
    virtual void Undo()    = 0;
};

}   // namespace jam
