#include "pch.h"
#include "Events.h"

ProjectChangedEvent::ProjectChangedEvent(
    const Ref<Project>& _pProject)
    : m_pProject(_pProject)
{
    JAM_ASSERT(m_pProject, "ProjectChangedEvent: _pProject cannot be null.");
}

const Ref<Project>& ProjectChangedEvent::GetProjectOrNull() const
{
    return m_pProject;
}

String ProjectChangedEvent::ToString() const
{
    return String::Formatted("ProjectChangedEvent: project name = '{}'", m_pProject ? m_pProject->GetName() : "null");
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
    return String::Formatted("ProjectRenamedEvent: old name = '{}', new name = '{}'", m_oldPath, m_newPath);
}

FocusingShaderItemChangedEvent::FocusingShaderItemChangedEvent(
    const Ref<ShaderItem>& _pShaderItem)
    : m_pShaderItem(_pShaderItem)
{
}

const Ref<ShaderItem>& FocusingShaderItemChangedEvent::GetShaderItemOrNull() const
{
    return m_pShaderItem;
}

String FocusingShaderItemChangedEvent::ToString() const
{
    return String::Formatted("FocusingShaderItemChangedEvent: shader item name = '{}'", m_pShaderItem ? m_pShaderItem->GetName() : "null");
}
