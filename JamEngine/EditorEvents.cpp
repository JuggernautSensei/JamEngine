#include "pch.h"
#include "EditorEvents.h"
#include "FileEntry.h"

namespace Jam
{

FocusingEntityChangedEvent::FocusingEntityChangedEvent(
    const Entity _entity)
    : m_entity(_entity)
{
}

Entity FocusingEntityChangedEvent::GetEntity() const
{
    return m_entity;
}

String FocusingEntityChangedEvent::ToString() const
{
    return String::Formatted("FocusingEntityChangedEvent: focusingEntity UID = {}", m_entity.GetUID());
}

FocusingFileEntryChangedEvent::FocusingFileEntryChangedEvent(
    const Ref<FileEntry>& _pFileEntry)
    : m_pEntry(_pFileEntry)
{
}

const Ref<FileEntry>& FocusingFileEntryChangedEvent::GetFileEntry() const
{
    return m_pEntry;
}

String FocusingFileEntryChangedEvent::ToString() const
{
    return String::Formatted("FocusingFileEntryChangedEvent: focusingFileEntry = {}", m_pEntry ? m_pEntry->path : "null");
}

FocusingDirEntryChangedEvent::FocusingDirEntryChangedEvent(
    const Ref<DirEntry>& _pEntry)
{
    m_pEntry = _pEntry;
}

const Ref<DirEntry>& FocusingDirEntryChangedEvent::GetDirEntry() const
{
    return m_pEntry;
}

String FocusingDirEntryChangedEvent::ToString() const
{
    return String::Formatted("FocusingDirEntryChangedEvent: focusingDirEntry = {}", m_pEntry ? m_pEntry->path : "null");
}

}   // namespace Jam