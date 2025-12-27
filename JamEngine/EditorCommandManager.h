#pragma once
#include "HistoryStack.h"

namespace jam
{

class IEditorCommand;

class EditorCommandManager
{
    static constexpr Int32 k_numCommandsLimit = 100;
    using CommandHistoryStack                 = FixedHistoryStack<Scoped<IEditorCommand>, k_numCommandsLimit>;

public:
    EditorCommandManager()  = default;
    ~EditorCommandManager() = default;

    EditorCommandManager(const EditorCommandManager&)            = delete;
    EditorCommandManager(EditorCommandManager&&)                 = delete;
    EditorCommandManager& operator=(const EditorCommandManager&) = delete;
    EditorCommandManager& operator=(EditorCommandManager&&)      = delete;

    void Execute(Scoped<IEditorCommand>&& _command);
    void Undo();
    void Redo();

    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;

    void Clear();

    [[nodiscard]] Int32 NumCommands() const;
    [[nodiscard]] Int32 NumCommandsLimit() const;

private:
    CommandHistoryStack m_historyStack = {};
};

}   // namespace jam
