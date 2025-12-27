#include "pch.h"
#include "EditorCommandManager.h"
#include "IEditorCommand.h"

namespace jam
{

void EditorCommandManager::Execute(
    Scoped<IEditorCommand>&& _command)
{
    _command->Execute();
    m_historyStack.Push(std::move(_command));
}

void EditorCommandManager::Undo()
{
    JAM_ASSERT(CanUndo(), "EditorCommandManager::Undo() - Cannot undo.");
    m_historyStack.GetCurrent()->Undo();
    m_historyStack.Undo();
}

void EditorCommandManager::Redo()
{
    JAM_ASSERT(CanRedo(), "EditorCommandManager::Redo() - Cannot redo.");
    m_historyStack.Redo();
    m_historyStack.GetCurrent()->Execute();
}

bool EditorCommandManager::CanUndo() const
{
    return m_historyStack.CanUndo();
}

bool EditorCommandManager::CanRedo() const
{
    return m_historyStack.CanRedo();
}

void EditorCommandManager::Clear()
{
    m_historyStack.Clear();
}

Int32 EditorCommandManager::NumCommands() const
{
    return m_historyStack.NumItems();
}

Int32 EditorCommandManager::NumCommandsLimit() const
{
    return k_numCommandsLimit;
}

}   // namespace jam