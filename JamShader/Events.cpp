#include "pch.h"
#include "Events.h"
#include "Resources.h"
#include "FileBrowserWindow.h"

ProjectChangedEvent::ProjectChangedEvent(
    Project* _pProject)
{
    m_pProject = _pProject;
}

Project* ProjectChangedEvent::GetProjectOrNull() const
{
    return m_pProject;
}

String ProjectChangedEvent::ToString() const
{
    return String::Formatted("ProjectChangedEvent: project filename = '{}'", m_pProject ? m_pProject->GetBaseName() : "null");
}

ProjectRenamedEvent::ProjectRenamedEvent(
    const StringView _oldPath,
    const StringView _newPath)
    : m_oldPath(_oldPath)
    , m_newPath(_newPath)
{
}

StringView ProjectRenamedEvent::GetOldPath() const
{
    return m_oldPath;
}

StringView ProjectRenamedEvent::GetNewPath() const
{
    return m_newPath;
}

String ProjectRenamedEvent::ToString() const
{
    return String::Formatted("ProjectRenamedEvent: old filename = '{}', new filename = '{}'", m_oldPath, m_newPath);
}

FocusingFileItemChangedEvent::FocusingFileItemChangedEvent(
    FileItem* _pFocusingFileItem)
    : m_pFocusingFileItem(_pFocusingFileItem)
{
}

FileItem* FocusingFileItemChangedEvent::GetFocusingFileItemOrNull() const
{
    return m_pFocusingFileItem;
}

String FocusingFileItemChangedEvent::ToString() const
{
    return String::Formatted("FocusingFileItemChangedEvent: focusing file item filename = '{}'", m_pFocusingFileItem ? m_pFocusingFileItem->filename : "null");
}

FocusingShaderItemChagnedEvent::FocusingShaderItemChagnedEvent(
    const StringView _shaderItemName)
    : m_shaderItemName(_shaderItemName)
{
}

StringView FocusingShaderItemChagnedEvent::GetFocusingShaderItemNameOrEmpty() const
{
    return m_shaderItemName;
}

String FocusingShaderItemChagnedEvent::ToString() const
{
    return String::Formatted("FocusingShaderItemChagnedEvent: focusing shader item name = '{}'", m_shaderItemName);
}
